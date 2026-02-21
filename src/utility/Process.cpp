#include "Process.hpp"

#include <algorithm>
#include <sstream>

#include "Argument.hpp"
#include "Settings.hpp"
#include "StringTools.hpp"
#include "base.hpp"

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE Pipe;
struct ProcessInfo
{
  HANDLE process;
  HANDLE thread;
};
#elif __unix__
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
typedef int Pipe;
struct ProcessInfo
{
  pid_t process;
  pid_t thread;
};
#endif

static void DumpPipeStr(Pipe pipe, std::ostream *out);
static const char *Process_GetErrorMessage();

static void KillProcess(const ProcessInfo &info);

static string join_args(const Blob<const Process::char_type *const> &args);

Process::Process(int argc, const char_type *const *argv)
    : Process(join_args({ argv, static_cast<size_t>(argc) })) {}

Process::Process(const char_type *cmd) : Process(std::string{ cmd }) {}

Process::Process(const std::string &cmd) : m_cmd{ cmd } {
  m_name = _BuildPrintableCMD(m_cmd.c_str());
}

int Process::start(std::ostream *const out) {
  int exit_code = -1;

#ifdef _WIN32
  Pipe output_r = {};
  Pipe output_w = {};

  if (out)
  {

    SECURITY_ATTRIBUTES sec_attrs = {};
    sec_attrs.nLength = sizeof(sec_attrs);
    sec_attrs.bInheritHandle = true;
    sec_attrs.lpSecurityDescriptor = nullptr;

    CreatePipe(&output_r, &output_w, &sec_attrs,
               Settings::Get("process_pipe_buffer_sz", 0x100000LL).get_int());
  }

  STARTUPINFOA startup_info = {};
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.hStdOutput = output_w;
  startup_info.hStdError = output_w;

  PROCESS_INFORMATION win_process_info = {};

  std::string cmd_copy = m_cmd;
  const bool create_proc_result =
      CreateProcessA(nullptr, cmd_copy.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                     &startup_info, &win_process_info);

  if (!create_proc_result)
  {
    Logger::error("creating process named '%s' error: %s", m_name.c_str(),
                  Process_GetErrorMessage());
    Logger::verbose("PROC-CMD:: %s", m_cmd.c_str());

    return -1;
  }

  ProcessInfo process_info = { win_process_info.hProcess, win_process_info.hThread };

  const DWORD wait_ms_timeout = this->m_wait_time_ms;
  const DWORD object_waiting_res = WaitForSingleObject(process_info.thread, wait_ms_timeout);

  if (object_waiting_res == WAIT_FAILED)
  {
    Logger::warning("waiting for process '%s' for %ums error: %s", m_name.c_str(), wait_ms_timeout,
                    Process_GetErrorMessage());
  }

  if (object_waiting_res == WAIT_TIMEOUT)
  {
    Logger::warning("waiting for process '%s' for %ums error: %s", m_name.c_str(), wait_ms_timeout,
                    Process_GetErrorMessage());
  }

  for (uint32_t i = 0; i < 64; i++)
  {
    const bool exit_code_result =
        GetExitCodeProcess(process_info.process, reinterpret_cast<DWORD *>(&exit_code));

    if (!exit_code_result)
    {
      Logger::error("getting exit code for process '%s' error: %s", m_name.c_str(),
                    Process_GetErrorMessage());
      break;
    }
    break;
  }

  // make sure the process is killed?
  KillProcess(process_info);
  CloseHandle(output_w);

  if (output_w && out)
  {
    DumpPipeStr(output_r, out);
  }

  CloseHandle(output_r);
#else
  Pipe child_pipes[2] = {};

  if (out)
  {
    if (pipe(child_pipes) == -1)
    {
      errno_t err = errno;
      Logger::error("Failed to create pipe for process '%s', ERRNO=%s", m_name.c_str(),
                    strerrorname_np(err));

      return err;
    }
  }

  std::vector<std::string> args = Argument::BreakArgumentList(m_cmd);

  if (args.empty())
  {
    Logger::error("No argument to start process, command=%s", m_cmd.c_str());
    return EFAULT;
  }

  std::vector<char *> arg_ptrs = {};
  for (size_t i = 0; i < args.size(); i++)
  {
    arg_ptrs.emplace_back(args[i].data());
  }
  arg_ptrs.push_back(nullptr);

  posix_spawn_file_actions_t sfc = { 0 };
  posix_spawn_file_actions_init(&sfc);

  posix_spawn_file_actions_addclose(&sfc, child_pipes[0]);
  posix_spawn_file_actions_adddup2(&sfc, child_pipes[1], STDOUT_FILENO);
  posix_spawn_file_actions_adddup2(&sfc, child_pipes[1], STDERR_FILENO);
  posix_spawn_file_actions_addclose(&sfc, child_pipes[1]);

  pid_t child_pid = 0;
  std::string exc_abs_path = FilePath::FindExecutableInPATHEnv(args[0]).c_str();

  if (exc_abs_path.empty())
  {
    exc_abs_path = args[0];
    Logger::verbose("No absolute path for '%s'", args[0].c_str());
  }
  else if (has_flags(Flag_CWDOverride))
  {
    std::string parent_dir = FilePath(exc_abs_path).parent().c_str();
    posix_spawn_file_actions_addchdir_np(&sfc, parent_dir.c_str());
  }

  arg_ptrs[0] = exc_abs_path.data();

  Logger::verbose("spawning '%s'", exc_abs_path.c_str());

  char **env_variables = nullptr;

  if (has_flags(Flag_InheritEnv))
  {
    env_variables = environ;
  }

  const errno_t spawn_err =
      posix_spawnp(&child_pid, exc_abs_path.c_str(), &sfc, nullptr, arg_ptrs.data(), env_variables);

  if (spawn_err != 0)
  {
    Logger::error("Failed to spawn process named '%s', error: %s", m_name.c_str(),
                  strerrorname_np(spawn_err));

    return -1;
  }

  close(child_pipes[1]);
  waitpid(child_pid, &exit_code, 0);

  if (child_pipes[0] && out)
  {
    DumpPipeStr(child_pipes[0], out);
  }

  close(child_pipes[0]);
#endif

  return exit_code;
}

Process::ProcessName Process::_BuildPrintableCMD(const char *string) {
  ProcessName output = {};
  constexpr size_t max_copy_length = PrintableCMDLength - 3;

  for (size_t i = 0; i < max_copy_length; i++)
  {
    if (string[i] == 0)
    {
      return output;
    }

    output.append(string[i]);

    if (i == max_copy_length - 1)
    {
      output.append('.', 3);
    }
  }

  return output;
}

void DumpPipeStr(Pipe pipe, std::ostream *out) {
  if (out == nullptr)
  {
    return;
  }

  Logger::verbose("dumping pipe %llu to the stream %p", pipe, out);

  constexpr size_t buffer_size = 0x1000;
  char buffer[buffer_size + 1] = {};

#ifdef _WIN32
  DWORD read_sz = 0;
  while (::ReadFile(pipe, buffer, buffer_size, &read_sz, nullptr))
#else
  ssize_t read_sz = 0;
  while ((read_sz = read(pipe, buffer, buffer_size)))
#endif
  {
    if (read_sz <= 0)
    {
      break;
    }

    Logger::verbose("read %llu bytes from pipe %llu", read_sz, pipe);

    buffer[read_sz] = 0;
    (*out) << buffer;
  }
}

const char *Process_GetErrorMessage() {
#ifdef _WIN32
  return GetErrorName(GetLastError());
#elif __linux__
  return strerrorname_np(errno);
#endif
}

void KillProcess(const ProcessInfo &info) {
#ifdef _WIN32
  CloseHandle(info.process);
  CloseHandle(info.thread);
#elif __linux__
  kill(info.process, SIGTERM);
#endif
}

string join_args(const Blob<const Process::char_type *const> &args) {
  std::ostringstream oss;

  for (const Process::char_type *arg : args)
  {
    bool need_enclosure =
        string_tools::all_of(arg, [](Process::char_type value) { return !isgraph(value); });

    if (need_enclosure)
    {
      oss << '"' << arg << '"';
    }
    else
    {
      oss << arg;
    }

    oss << ' ';
  }

  return oss.str();
}
