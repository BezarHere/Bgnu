#pragma once

#include "Command.hpp"

class SourceProcessor;

namespace commands
{

  class BuildCommand : public Command
  {
  public:
    inline BuildCommand() : Command("build", "builds a project") {}

    Error execute(ArgumentSource &reader) override;
    Error get_help(ArgumentSource &reader, string &out) override;
    inline CommandInfo get_info() const override {
      return { "build",
               "builds the project in the working directory, project files are named '.bgnu'" };
    }
  };

}
