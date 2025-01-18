#include "Project.hpp"
#include "FieldDataReader.hpp"
#include "io/FieldWriter.hpp"
#include "BuildTools.hpp"


Project Project::GetDefault() {
  Project project = {};

  project.m_build_configurations->try_emplace(
    "debug", BuildConfiguration::GetDefault(BuildConfigurationDefaultType::Debug)
  );

  project.m_build_configurations->try_emplace(
    "release", BuildConfiguration::GetDefault(BuildConfigurationDefaultType::Release)
  );

  return project;
}

Project Project::from_data(const FieldVar::Dict &data, ErrorReport &result) {
  Project project{};

  FieldDataReader reader{"Project", data};

  result = load_various(project, reader);
  if (result)
  {
    Logger::error(result);
    return project;
  }

  const FieldVar &build_configs = reader.try_get_value<FieldVarType::Dict>(
    project.m_build_configurations.name()
  );

  if (build_configs.is_null())
  {
    result.code = Error::NoData;
    result.message = "No build configs ('build_configurations') set";
    Logger::error("Project: %s", to_cstr(result.message));
    return project;
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
        to_cstr(value.get_type_name())
      );

      result.code = Error::InvalidType;
      result.message = format_join("ill-typed build configuration \"", key, "\"");
      return project;
    }


    BuildConfiguration config = BuildConfiguration::from_data(
      FieldDataReader(
        format_join(reader.get_context(), "::", "BuildCfg[", key, ']'),
        value.get_dict()
      ), result
    );

    if (result.code != Error::Ok)
    {
      Logger::debug("Failure in parsing and loading build cfg named '%s'", to_cstr(key));
      return project;
    }

    project.m_build_configurations->insert_or_assign(key, config);
  }

  return project;
}

errno_t Project::to_data(const Project &project, FieldVar &output) {
  if (output.get_type() != FieldVarType::Dict)
  {
    output = FieldVar(FieldVarType::Dict);
  }

  FieldWriter writer = {};

  FieldVar::Dict glob_selectors = {};

  for (const auto &item : project.m_source_selectors)
  {
    glob_selectors.try_emplace(
      item.glob.get_source().c_str(),
      build_tools::GetSourceFileTypeName(item.type)
    );
  }

  writer.write(
    "source_selectors",
    glob_selectors
  );

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
      Logger::error(
        "error while serializing config named '%s' [%d]: %s",
        name.c_str(), (int)report.code, report.message.c_str()
      );
    }
  }

  writer.write(project.m_build_configurations.name(), config_dicts);

  output = FieldVar(writer.output);

  return EOK;
}

ErrorReport Project::load_various(Project &project, FieldDataReader &reader) {
  const FieldVar::Dict &src_slc = \
    reader.try_get_value<FieldVarType::Dict>("source_selectors", FieldVar(FieldVarType::Dict)).get_dict();

  if (src_slc.empty())
  {
    Logger::verbose("found no specified source selectors, using default...");
  }
  else
  {
    Logger::verbose("found %llu source selectors", src_slc.size());
    project.m_source_selectors.clear();
  }


  for (const auto &[var, type] : src_slc)
  {
    if (type.get_type() != FieldVarType::String)
    {
      Logger::error(
        "project source selector [key='%s'] should have a value of type STRING defining the glob's (key) type format (e.x. 'c' or 'c++' or ...)",
        var.c_str()
      );

      continue;
    }

    const SourceFileType parsed_type = build_tools::GetFileTypeFromTypeName(type.get_string());

    if (parsed_type == SourceFileType::None)
    {
      Logger::error(
        "project source selector [key='%s'] has an unknown glob's/key's type/value='%s' (e.x. 'c' or 'c++' or ...)",
        var.c_str(),
        type.get_string().c_str()
      );
      continue;
    }

    project.m_source_selectors.emplace_back(Glob(var), parsed_type);
  }

  const FieldVar &output_dir = reader.try_get_value<FieldVarType::String>(
    FieldIO::NestedName(project.m_output.name(), project.m_output->dir)
  );

  if (output_dir.is_null())
  {
    Logger::verbose(
      "no specified output directory, using default: '%s'",
      project.m_output->dir->c_str()
    );
  }
  else
  {
    project.m_output->dir.field() = output_dir.get_string();
  }

  const FieldVar &output_cache_dir = reader.try_get_value<FieldVarType::String>(
    FieldIO::NestedName(project.m_output.name(), project.m_output->cache_dir)
  );
  if (output_cache_dir.is_null())
  {
    Logger::verbose(
      "no specified cache directory, using default: '%s'",
      project.m_output->cache_dir->c_str()
    );
  }
  else
  {
    project.m_output->cache_dir.field() = output_cache_dir.get_string();
  }

  const FieldVar &output_name = reader.try_get_value<FieldVarType::String>(
    FieldIO::NestedName(project.m_output.name(), project.m_output->name)
  );
  if (output_name.is_null())
  {
    Logger::verbose(
      "no specified output name, using default: '%s'",
      project.m_output->name->c_str()
    );
  }
  else
  {
    project.m_output->name.field() = output_name.get_string();
  }

  return ErrorReport();
}

vector<FilePath::iterator_entry> Project::get_available_files() const {
  vector<FilePath::iterator_entry> entries{};
  vector<FilePath::iterator> iterators{};

  entries.reserve(1024);

  iterators.push_back(this->source_dir.create_iterator());

  while (!iterators.empty())
  {
    const auto current_iter = iterators.back();
    iterators.pop_back();

    for (const auto &path : current_iter)
    {
      if (path.is_directory())
      {
        iterators.emplace_back(path);
        continue;
      }

      if (path.is_regular_file())
      {
        entries.push_back(path);
        continue;
      }

      // skips other non-regular files
    }
  }

  return entries;
}

vector<FilePath> Project::get_source_files() const {
  vector<FilePath> result_paths{};

  for (const auto &p : get_available_files())
  {
    FilePath path = p;


    if (is_matching_source(path.get_text()))
    {
      result_paths.push_back(path);
    }

  }

  return result_paths;
}

bool Project::is_matching_source(const StrBlob &path) const {
  return get_source_type(path) != SourceFileType::None;
}

SourceFileType Project::get_source_type(const StrBlob &path) const {
  for (const SourceTypeGlob &stg : m_source_selectors)
  {
    if (stg.glob.test(path))
    {
      return stg.type;
    }
  }
  return SourceFileType::None;
}

SourceFileType Project::get_source_type_or_default(const StrBlob &path) const {
  const SourceFileType type = get_source_type(path);

  if (type == SourceFileType::None)
  {
    return build_tools::DefaultSourceFileTypeForFilePath(path);
  }

  return type;
}

hash_t Project::hash_own(HashDigester &digester) const {
  digester += m_output.field();

  for (const SourceTypeGlob &stg : m_source_selectors)
  {
    digester.add((hash_t)stg.type);
    digester.add(StrBlob{stg.glob.get_source().c_str(), stg.glob.get_source().length()});
  }

  return digester.value;
}

hash_t Project::hash_cfgs(HashDigester &digester) const {

  for (const auto &[name, cfg] : m_build_configurations.field())
  {
    digester.add(StrBlob{name.c_str(), name.length()});
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
  this->name->parent().create_directory();
  this->cache_dir->create_directory();
  this->dir->create_directory();
  return Error::Ok;
}

FilePath ProjectOutputData::get_result_path() const {
  return FilePath(this->name.field()).resolve(this->dir->resolved_copy());
}
