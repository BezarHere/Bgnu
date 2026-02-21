#include "Logger.hpp"
#include "Startup.hpp"
// #include <Windows.h>

namespace chrono
{
  using namespace std::chrono;
}

int main(int argc, const char *argv[]) {
  const chrono::time_point pre_run_time = chrono::steady_clock::now();

#ifdef __linux__
  extern char **environ;

#endif

  int result = Startup::start(ArgumentSource(argv + 1, std::max(argc - 1, 0)));

  const auto run_time =
      chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - pre_run_time);

  Logger::write_raw("-- run time: %.2fs --\n",
                    (float)run_time.count() / (float)decltype(run_time)::period::den);

  return result;
}
