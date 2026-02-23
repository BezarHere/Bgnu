#include "BuildCommand.hpp"

#include <utility/ThreadBatcher.hpp>

#include "ProjectService.hpp"
#include "code/SourceProcessor.hpp"

typedef SourceProcessor::dependency_map dependency_map;
typedef vector<vector<string>> build_args_list;

namespace commands
{
  Error BuildCommand::execute(ArgumentSource &reader) {
    ProjectService::SetArguments(reader);
    return ProjectService::ExecuteStepsTo(BuildStep::PostLinking).first;
  }

  Error BuildCommand::get_help(ArgumentSource &reader, string &out) {
    out.append("usage: build [-r/--rebuild] [--resave] [-m=<build mode>/--mode=<build mode>]\n");

    out.append("[-r/--rebuild]:\n")
        .append(
            "  rebuilds the current project, deleting all the cached object files (+ it's "
            "folder)\n");

    out.append("[--resave]:\n")
        .append("  saves the project bake to the project file (for fixing project files)\n")
        .append("  project file is the '.bgnu' in the current working directory\n");

    out.append("[-m=<build mode>/--mode=<build mode>]:\n")
        .append("  sets the build mode (i.e. 'release' or 'debug')\n")
        .append("  defaults to 'debug'\n")
        .append(
            "  if no build configuration has the given '<build mode>' name, an error is thrown\n");

    return Error();
  }

}