#include "RunCommand.hpp"

#include <Settings.hpp>

#include "BuildTools.hpp"
#include "ProjectService.hpp"


Error commands::RunCommand::execute(ArgumentSource &reader) {
  Logger::verbose("do a run-build...");

  std::vector<Argument> build_args = {};
  std::vector<std::string> run_args = {};

  Logger::verbose("forwarding build args...");
  while (!reader.is_empty())
  {
    if (reader.peek().get_value() == "--run")
    {
      break;
    }

    build_args.push_back(Argument(reader.read().get_value()));
    Logger::verbose("+ forwarded to build: \"%s\"", build_args.back().get_value().c_str());
  }

  if (reader.peek().get_value() == "--run")
  {
    reader.read();

    while (!reader.is_empty())
    {
      run_args.push_back(reader.read().get_value());
      Logger::verbose("+ forwarded to run: \"%s\"", run_args.back().c_str());
    }
  }

  Logger::verbose("creating new forward arg source...");
  reader = ArgumentSource(Blob<const Argument>(build_args.data(), build_args.size()));

  Error err = CommandDB::get_command("build")->execute(reader);
  if (err != Error::Ok)
  {
    return err;
  }

  const FilePath path = ProjectService::GetLinkOutputPath();

  string system_cmd = {};
  system_cmd.push_back('"');
  system_cmd.append(path);
  system_cmd.push_back('"');

  const string args_joined = build_tools::JoinArguments(run_args.data(), run_args.size());

  if (!args_joined.empty())
  {
    system_cmd.append(" ");
    system_cmd.append(args_joined);
  }

  Logger::verbose("running cmd: \"%s\"", system_cmd.c_str());
  Logger::notify("[*] Running...");

  std::system(system_cmd.c_str());
}

Error commands::RunCommand::get_help(ArgumentSource &reader, string &out) {
  out =
      "builds using the configuration given in the settings file (defaults to 'debug')\n"
      "passes the all the arguments given to the build command after the '--run' argument\n"
      "example: 'bgnu run abc 123 \"hello world!\" -c' will be passed as\n"
      "\t'bgnu build -m=<configs from settings> --run abc 123 \"hello world!\" -c'";
  return Error::Ok;
}
