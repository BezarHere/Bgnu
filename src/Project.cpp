#include "Project.hpp"

#include "BuildTools.hpp"
#include "FieldDataReader.hpp"
#include "FieldVar.hpp"
#include "io/FieldWriter.hpp"

Project Project::GetDefault() {
  Project project = {};

  project.m_build_configurations->try_emplace(
      "debug",
      BuildConfiguration::GetDefault(BuildConfigurationDefaultType::Debug));

  project.m_build_configurations->try_emplace(
      "release",
      BuildConfiguration::GetDefault(BuildConfigurationDefaultType::Release));

  return project;
}

Project Project::from_data(const FieldVar::Dict &data, ErrorReport &result) {
  Project project{};

  FieldDataReader reader{ "Project", data };

  result = load_various(project, reader);
  if (result)
  {
    Logger::error(result);
    return project;
  }

  const FieldVar &build_configs =
      reader.try_get_value<FieldVarType::Dict>(project.m_build_configurations.name());

  if (build_configs.is_null())
  {
    result.code = Error::NoData;
    result.message = "No build configs ('build_configurations') set";
    Logger::error("Project: %s", to_cstr(result.message));
    return project;
  }

  const FieldVar clangd = reader.try_get_value<FieldVarType::Boolean>(project.clangd.name());
  if (!clangd.is_null())
  {
    *project.clangd = clangd.get_bool();
  }

  for (const auto &[key, value] : build_configs.get_dict())
  {
    Logger::debug("Started parsing build config: %s", to_cstr(key));
    if (value.get_type() != FieldVarType::Dict)
    {
      Logger::error(
          "%s: Invalid value for build configuration '%s': expected type %s, gotten type %s",
          to_cstr(reader.get_context()),
          to_cstr(key),
          to_cstr(FieldVar::get_name_for_type(FieldVarType::Dict)),
          to_cstr(value.get_type_name()));

      result.code = Error::InvalidType;
      result.message = format_join("ill-typed build configuration \"", key, "\"");
      return project;
    }

    BuildConfiguration config = BuildConfiguration::from_data(
        FieldDataReader(format_join(reader.get_context(), "::", "BuildCfg[", key, ']'),
                        value.get_dict()),
        result);

    if (result.code != Error::Ok)
    {
      Logger::debug("Failure in parsing and loading build cfg named '%s'", to_cstr(key));
      return project;
    }

    project.m_build_configurations->insert_or_assign(key, config);
  }

  const FieldVar &modifiers = reader.try_get_value<FieldVarType::Array>(project.m_modifiers.name());

  if (!modifiers.is_null())
  {
    for (size_t i = 0; i < modifiers.get_array().size(); i++)
    {
      const FieldVar &modifier_data = modifiers.get_array()[i];
      if (modifier_data.get_type() != FieldVarType::Dict)
      {
        Logger::warning(
            "Project::Modifier[%llu]: Modifier data should be of type Dict, found type %s",
            i,
            to_cstr(modifier_data.get_type_name()));
        continue;
      }

      Result<ProjectModifier> modifier = ProjectModifier::from_data(modifier_data.get_dict());
      if (!modifier)
      {
        Logger::warning("Project::Modifier[%llu]: %s", i, to_cstr(modifier.message()));
        continue;
      }

      project.m_modifiers->push_back(*modifier);
    }
  }

  return project;
}

errno_t Project::to_data(const Project &project, FieldVar &output) {
  if (output.get_type() != FieldVarType::Dict)
  {
    output = FieldVar(FieldVarType::Dict);
  }

  FieldWriter writer = {};

  // writer.write_nested_transform(project.m_output.name(), project.m_output->type);
  writer.write_nested(project.m_output.name(), project.m_output->name);
  writer.write_nested(project.m_output.name(), project.m_output->dir);
  writer.write_nested(project.m_output.name(), project.m_output->cache_dir);

  FieldVar::Dict config_dicts = {};
  for (const auto &[name, config] : project.m_build_configurations.field())
  {
    ErrorReport report = {};
    config_dicts.try_emplace(name, BuildConfiguration::to_data(config, report));

    if (report)
    {
      Logger::error("error while serializing config named '%s' [%d]: %s",
                    name.c_str(),
                    (int)report.code,
                    report.message.c_str());
    }
  }

  writer.write(project.clangd);
  writer.write(project.m_build_configurations.name(), config_dicts);

  output = FieldVar(writer.output);

  return EOK;
}

ErrorReport Project::load_various(Project &project, FieldDataReader &reader) {
  const FieldVar &output_dir = reader.try_get_value<FieldVarType::String>(
      FieldIO::NestedName(project.m_output.name(), project.m_output->dir));

  if (output_dir.is_null())
  {
    Logger::verbose("no specified output directory, using default: '%s'",
                    project.m_output->dir->c_str());
  }
  else
  {
    project.m_output->dir.field() = output_dir.get_string();
  }

  const FieldVar &output_cache_dir = reader.try_get_value<FieldVarType::String>(
      FieldIO::NestedName(project.m_output.name(), project.m_output->cache_dir));
  if (output_cache_dir.is_null())
  {
    Logger::verbose("no specified cache directory, using default: '%s'",
                    project.m_output->cache_dir->c_str());
  }
  else
  {
    project.m_output->cache_dir.field() = output_cache_dir.get_string();
  }

  const FieldVar &output_name = reader.try_get_value<FieldVarType::String>(
      FieldIO::NestedName(project.m_output.name(), project.m_output->name));
  if (output_name.is_null())
  {
    Logger::verbose("no specified output name, using default: '%s'",
                    project.m_output->name->c_str());
  }
  else
  {
    project.m_output->name.field() = output_name.get_string();
  }

  return ErrorReport();
}

vector<FilePath> Project::get_source_files() const {
  vector<FilePath> result_paths{};

  for (const auto &path : build_tools::GetAllFilesInDirectory(source_dir, true))
  {
    if (is_source_file_path(path.get_text()))
    {
      result_paths.push_back(path);
    }
  }

  return result_paths;
}

bool Project::is_source_file_path(const StrBlob &path) const {
  return build_tools::IsCompilableSourceFile(path) && build_tools::GetSourceType(path) != SourceFileType::None;
}

SourceFileType Project::get_source_type_or_default(const StrBlob &path) const {
  const SourceFileType type = build_tools::GetSourceType(path);

  if (type == SourceFileType::None)
  {
    return build_tools::DefaultSourceFileTypeForFilePath(path);
  }

  return type;
}

hash_t Project::hash_own(HashDigester &digester) const {
  digester += m_output.field();
  return digester.value;
}

hash_t Project::hash_cfgs(HashDigester &digester) const {

  for (const auto &[name, cfg] : m_build_configurations.field())
  {
    digester.add(StrBlob{ name.c_str(), name.length() });
    digester += cfg;
  }

  return digester.value;
}

hash_t Project::hash() const {
  HashDigester digester = {};

  hash_own(digester);
  hash_cfgs(digester);

  return digester.value;
}

Error ProjectOutputData::ensure_available() const {
  this->get_result_path().parent().create_directory();
  this->cache_dir->create_directory();
  this->dir->create_directory();
  return Error::Ok;
}

FilePath ProjectOutputData::get_result_path() const {
  return FilePath(this->name.field()).resolve(this->dir->resolved_copy());
}

using ModifierNamedType = std::pair<ProjectModifier::Type, std::string>;
static constexpr std::array<ModifierNamedType, 6> ModifierTypeNames = {
  ModifierNamedType{ ProjectModifier::Type::Set, "set" },
  ModifierNamedType{ ProjectModifier::Type::Del, "del" },
  ModifierNamedType{ ProjectModifier::Type::Add, "add" },
  ModifierNamedType{ ProjectModifier::Type::Sub, "sub" },
  ModifierNamedType{ ProjectModifier::Type::Mul, "mul" },
  ModifierNamedType{ ProjectModifier::Type::Div, "div" },
};

Result<ProjectModifier> ProjectModifier::from_data(const FieldVar::Dict &data) {
  if (!data.contains("type"))
  {
    return { Error::NoData, "Expecting 'type' field for modifier type (Add, )" };
  }
  if (!data.at("type").is_convertible_to(FieldVarType::String))
  {
    return { Error::InvalidType, "Expecting 'type' field to be a string" };
  }

  const auto &type_str = data.at("type").get_string();
  const Type type = NameToType(type_str);
  if (type == Type::None)
  {
    return { Error::Failure, format_join("Invalid type \"", type_str, "\"") };
  }

  if (!data.contains("target"))
  {
    return { Error::NoData, "Expecting 'target' field for modifier targets (build configs)" };
  }
  if (data.at("target").get_type() != FieldVarType::String)
  {
    return { Error::InvalidType, "Expecting 'target' field to be a string" };
  }

  const Glob target = { data.at("target").get_string() };

  if (!data.contains("property"))
  {
    return { Error::NoData, "Expecting 'property' field for modifier properties" };
  }
  if (data.at("property").get_type() != FieldVarType::String)
  {
    return { Error::InvalidType, "Expecting 'property' field to be a string (property name)" };
  }
  const auto property = data.at("property").get_string();

  if (!data.contains("value"))
  {
    return { Error::NoData, "Expecting 'value' field for modifier value" };
  }

  const FieldVar value = data.at("value");

  return ProjectModifier{ type, target, property, value };
}

auto ProjectModifier::NameToType(const std::string &str) -> Type {
  std::string new_str = string_tools::to_lower(str);
  for (const auto &[type, name] : ModifierTypeNames)
  {
    if (new_str == name)
    {
      return type;
    }
  }

  return Type::None;
}
