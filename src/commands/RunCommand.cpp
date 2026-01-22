#include "RunCommand.hpp"
#include <Settings.hpp>


Error commands::RunCommand::execute(ArgumentSource &reader) {
  Logger::verbose("do a run-build...");

  std::vector<Argument> args = {};

  args.emplace_back(
    "-m=" + Settings::Get("run_configuration", "debug").get_string()
  );

  args.emplace_back("--run");
  
  Logger::verbose("forwarding build args...");
  while (!reader.is_empty())
  {
    args.push_back(Argument(reader.read().get_value()));
    Logger::verbose("+ forwarded arg: \"%s\"", args.back().get_value().c_str());
  }
  
  Logger::verbose("creating new forward arg source...");
  reader = ArgumentSource(Blob<const Argument>(args.data(), args.size()));

  return CommandDB::get_command("build")->execute(reader);
}

Error commands::RunCommand::get_help(ArgumentSource &reader, string &out) {
  out = "builds using the configuration given in the settings file (defaults to 'debug')\n"
    "passes the all the arguments given to the build command after the '--run' argument\n"
    "example: 'bgnu run abc 123 \"hello world!\" -c' will be passed as\n"
    "\t'bgnu build -m=<configs from settings> --run abc 123 \"hello world!\" -c'";
  return Error::Ok;
}


