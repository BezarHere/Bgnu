#pragma once

#include "Command.hpp"

class SourceProcessor;

namespace commands
{

  class MapCommand : public Command
  {
  public:
    inline MapCommand() : Command("map", "maps a project") {}

    Error execute(ArgumentSource &reader) override;
    Error get_help(ArgumentSource &reader, string &out) override;
    inline CommandInfo get_info() const override {
      return { "map",
               "maps a project without building, adding a clangd flags and "
               "compile commands files" };
    }
  };

}
