#pragma once
#include "FieldIO.hpp"
#include "utility/NField.hpp"

struct FieldWriter : public FieldIO
{
  template <typename T>
  void write_arr(const std::string &name, const Blob<const T> &generic_arr);

  template <typename T>
  void write(const std::string &name, const T &var);

  template <typename T>
  inline void write(const NField<T> &field) {
    return write(field.name(), field.field());
  }

  template <typename T>
  inline void write_nested(const std::string &parent, const NField<T> &field) {
    return write(NestedName(parent, field), field.field());
  }

  template <typename T, typename Transformer>
  inline void write_nested_transform(const std::string &parent, const NField<T> &field,
                                     Transformer &&trn) {
    return write(NestedName(parent, field), trn(field.field()));
  }

  FieldVar::Dict output;
};

template <typename T>
inline void FieldWriter::write_arr(const std::string &name, const Blob<const T> &generic_arr) {
  FieldVar::Array &arr =
      this->output.try_emplace(name, FieldVar{ FieldVarType::Array }).first->second.get_array();

  for (const auto &obj : generic_arr)
  {
    arr.emplace_back(obj);
  }
}

template <typename T>
inline void FieldWriter::write(const std::string &name, const T &var) {
  this->output.try_emplace(name, FieldVar{ var });
}
