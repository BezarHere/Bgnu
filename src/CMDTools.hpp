#pragma once
#include "base.hpp"

struct CMDTools
{
  static int execute(const string_char *cmd_line);
  static void execute_pool(const Blob<string> &commands, vector<int> &ret_codes);
};
