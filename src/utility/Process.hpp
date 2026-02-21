#pragma once
#include <inttypes.h>

#include <array>
#include <string>

#include "misc/StaticString.hpp"

class Process
{
public:
  static constexpr size_t PrintableCMDLength = 63;
  typedef char char_type;
  typedef StaticString<PrintableCMDLength> ProcessName;

  typedef uint16_t ProcessFlags;

  static constexpr ProcessFlags Flag_None = 0x0000;
  static constexpr ProcessFlags Flag_CWDOverride = 0x0001;
  static constexpr ProcessFlags Flag_InheritEnv = 0x0002;

  Process(int argc, const char_type *const *argv);
  Process(const char_type *cmd);
  Process(const std::string &cmd);

  int start(std::ostream *out = nullptr);

  inline const ProcessName &get_name() const { return m_name; }

  inline ProcessFlags get_flags() const { return m_flags; }
  inline ProcessFlags has_flags(ProcessFlags mask) const { return m_flags & mask; }

  inline void set_name(const ProcessName &name) { m_name = name; }

  inline void set_flags(ProcessFlags flags) { m_flags = flags; }

  inline void add_flags(ProcessFlags flags) { m_flags |= flags; }
  inline void remove_flags(ProcessFlags flags) { m_flags &= ~flags; }

private:
  static ProcessName _BuildPrintableCMD(const char *string);

private:
  std::string m_cmd;
  ProcessName m_name = {};
  ProcessFlags m_flags = Flag_None;
  unsigned long m_wait_time_ms = 1000 * 60;  // 1 minutes
};
