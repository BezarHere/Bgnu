#include "Process.hpp"
#include "base.hpp"
#include "StringTools.hpp"
#include "Settings.hpp"

#include <algorithm>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE Pipe;
struct ProcessInfo
{
	HANDLE process;
	HANDLE thread;
};
#elif __unix__
// TODO: unix stuff

typedef _Filet *Pipe;
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
	: Process(join_args({argv, argc})) {
}

Process::Process(const char_type *cmd)
	: Process(std::string{cmd}) {
}

Process::Process(const std::string &cmd)
	: m_cmd{cmd} {
	m_printable_cmd = _BuildPrintableCMD(m_cmd.c_str());
}

int Process::start(std::ostream *const out) {
	int exit_code = -1;
	Pipe output_r = {};
	Pipe output_w = {};

#ifdef _WIN32

	if (out)
	{

		SECURITY_ATTRIBUTES sec_attrs = {};
		sec_attrs.nLength = sizeof(sec_attrs);
		sec_attrs.bInheritHandle = true;
		sec_attrs.lpSecurityDescriptor = nullptr;

		CreatePipe(
			&output_r, &output_w, &sec_attrs, Settings::Get("process_pipe_buffer_sz", 0x1A000LL).get_int()
		);
	}

	STARTUPINFOA startup_info = {};
	startup_info.cb = sizeof(startup_info);
	startup_info.dwFlags = STARTF_USESTDHANDLES;
	startup_info.hStdOutput = output_w;
	startup_info.hStdError = output_w;

	PROCESS_INFORMATION win_process_info = {};

	std::string cmd_copy = m_cmd;
	const bool create_proc_result = CreateProcessA(
		nullptr, cmd_copy.data(),
		nullptr, nullptr,
		TRUE, 0,
		nullptr, nullptr,
		&startup_info, &win_process_info
	);

	if (!create_proc_result)
	{
		Logger::error(
			"creating process '%s' error: %s",
			m_printable_cmd.data(),
			Process_GetErrorMessage()
		);

		return -1;
	}

	ProcessInfo process_info = {
		win_process_info.hProcess,
		win_process_info.hThread
	};

	const DWORD wait_ms_timeout = INFINITE;
	const DWORD object_waiting_res = WaitForSingleObject(
		process_info.process, wait_ms_timeout
	);

	if (object_waiting_res == WAIT_FAILED)
	{
		Logger::warning(
			"waiting for process '%s' for %ums error: %s",
			m_printable_cmd.data(),
			wait_ms_timeout,
			Process_GetErrorMessage()
		);
	}

	for (uint32_t i = 0; i < 64; i++)
	{
		const bool exit_code_result = GetExitCodeProcess(
			process_info.process,
			reinterpret_cast<DWORD *>(&exit_code)
		);

		if (!exit_code_result)
		{
			Logger::error(
				"getting exit code for process '%s' error: %s",
				m_printable_cmd.data(),
				Process_GetErrorMessage()
			);
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

#endif


	return exit_code;
}

Process::PrintableBuffer Process::_BuildPrintableCMD(const char *string) {
	PrintableBuffer output = {};
	constexpr size_t max_copy_length = PrintableCMDLength - 3;

	for (size_t i = 0; i < max_copy_length; i++)
	{
		output[i] = string[i];

		if (string[i] == 0)
		{
			return output;
		}

		if (i == max_copy_length - 1)
		{
			output[++i] = '.';
			output[++i] = '.';
			output[++i] = '.';
			output[++i] = 0;
		}
	}

	memcpy(output.data(), string, max_copy_length);
	output[PrintableCMDLength] = 0;
	return output;
}

void DumpPipeStr(Pipe pipe, std::ostream *out) {
	if (out == nullptr)
	{
		return;
	}

	Logger::verbose("dumping pipe %llu to the stream %p", pipe, out);

	constexpr size_t buffer_size = 1024;
	char buffer[buffer_size + 1] = {};
	DWORD read_sz = 0;

	while (::ReadFile(pipe, buffer, buffer_size, &read_sz, nullptr))
	{
		if (read_sz == 0)
		{
			break;
		}


		Logger::verbose("read %llu bytes from pipe %llu", read_sz, pipe);

		buffer[read_sz] = 0;
		(*out) << buffer;
	}


}

const char *Process_GetErrorMessage() {
	return GetErrorName(GetLastError());
}

void KillProcess(const ProcessInfo &info) {
	CloseHandle(info.process);
	CloseHandle(info.thread);
}

string join_args(const Blob<const Process::char_type *const> &args) {
	std::ostringstream oss;

	for (const Process::char_type *arg : args)
	{
		bool need_enclosure = string_tools::all_of(
			arg,
			[](Process::char_type value) {return !isgraph(value);}
		);

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
