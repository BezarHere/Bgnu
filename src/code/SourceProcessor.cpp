#include "SourceProcessor.hpp"
#include "FileTools.hpp"
#include <set>

void SourceProcessor::process() {
	while (!m_input_stack.empty())
	{
		const InputFilePath input = m_input_stack.top();
		m_input_stack.pop();
		_process_input(input);
	}
}

void SourceProcessor::add_file(const InputFilePath &input) {
	m_inputs.push_back(input);
	m_input_stack.push(input);

	if (m_input_stack.top().source_type == SourceFileType::None)
	{
		m_input_stack.top().source_type = SourceTools::get_default_file_type(m_input_stack.top().path);
	}
}

hash_t SourceProcessor::get_file_hash(const FilePath &filepath) const {
	return m_info_map.at(filepath).hash;
}

bool SourceProcessor::has_file_hash(const FilePath &filepath) const {
	return m_info_map.find(filepath) != m_info_map.end();
}

SourceProcessor::file_change_list SourceProcessor::gen_file_change_table(bool inputs_only) const {

	std::set<FilePath> file_paths;

	if (inputs_only)
	{
		for (const InputFilePath &input : m_inputs)
		{
			file_paths.insert(input.path);
		}
	}
	else
	{
		for (const auto &kv : m_info_map)
		{
			file_paths.insert(kv.first);
		}
	}

	file_change_list list{};
	for (const FilePath &key : file_paths)
	{
		const auto iter_pos = this->file_records.find(key);
		const DependencyInfo &value = m_info_map.at(key);

		// no record of this file
		if (iter_pos == file_records.end())
		{
			list.emplace_back(key, value.hash);
		}

		// record has an invalid hash
		if (iter_pos->second.hash != value.hash)
		{
			list.emplace_back(key, value.hash);
		}
	}

	return list;
}

SourceProcessor::dependency_map SourceProcessor::gen_dependency_map() const {
	dependency_map map;
	for (const auto &[key, value] : m_info_map)
	{
		map[key] = value.sub_dependencies;
	}
	return map;
}


bool SourceProcessor::_is_processing_path(const FilePath &filepath) const {
	return m_loading_stack.top() == filepath;
}

void SourceProcessor::_push_processing_path(const FilePath &filepath) {
	m_loading_stack.push(filepath);
}

void SourceProcessor::_pop_processing_path(const FilePath &filepath) {
	if (m_loading_stack.empty())
	{
		Logger::error(
			"expecting \"%s\" to be the top in the loading stack, but the loading stack is empty",
			filepath.c_str()
		);
		return;
	}

	if (m_loading_stack.top() != filepath)
	{
		Logger::error(
			"Ill-loading stack: expected the top to be \"%s\", but it is \"%s\"",
			filepath.c_str(),
			m_loading_stack.top().c_str()
		);
		return;
	}

	m_loading_stack.pop();
}

void SourceProcessor::_process_input(const InputFilePath &input) {
	DependencyInfo &dep_info = m_info_map.insert_or_assign(input.path, DependencyInfo()).first->second;
	dep_info.type = input.source_type;

	_push_processing_path(input.path);

	const Buffer _raw_input_buf = FileTools::read_all(input.path, FileTools::FileKind::Text);

	SourceTools::get_dependencies(
		_raw_input_buf.to_blob<const string_char>(),
		dep_info.type,
		dep_info.sub_dependencies
	);

	HashDigester hash_digest;

	for (size_t i = 0; i < dep_info.sub_dependencies.size(); i++)
	{
		FilePath dependency_path = _find_dependency(
			dep_info.sub_dependencies[i],
			input.path,
			dep_info.type
		);

		if (!dependency_path.exists())
		{
			if (has_flags(eFlag_WarnAbsentDependencies))
			{
				Logger::warning(
					"dependency named \"%s\" couldn't be found for file at \"%s\"",
					dep_info.sub_dependencies[i].c_str(),
					input.path.c_str()
				);
			}
			continue;
		}

		// the path is already processed or being processed (from a recursive caller)
		// we do this to eliminate inf recursion
		if (has_file_hash(dependency_path))
		{
			hash_digest += get_file_hash(dependency_path);
			continue;
		}

		InputFilePath sub_input;
		sub_input.path = dependency_path;
		sub_input.source_type = SourceTools::get_default_file_type(sub_input.path);

		// sanity checks (might be bad)
		if (SourceTools::is_compatable_types(sub_input.source_type, input.source_type))
		{
			sub_input.source_type = input.source_type;
		}

		// process sub dependency
		_process_input(sub_input);
		// add it's hash
		hash_digest += get_file_hash(dependency_path);
	}

	dep_info.hash = hash_digest.value;

	_pop_processing_path(input.path);
}

FilePath SourceProcessor::_find_dependency(const dependency_name &name,
																					 const FilePath &file,
																					 SourceFileType type) const {

	// is the dependency path local to the file?

	FilePath local_path = FilePath(name, file.parent());

	if (_dependency_exists(local_path, type))
	{
		return local_path;
	}


	for (const auto &dir : this->included_directories)
	{
		FilePath dir_path = FilePath(name, dir);
		if (_dependency_exists(dir_path, type))
		{
			return dir_path;
		}
	}

	return {};
}

bool SourceProcessor::_dependency_exists(const FilePath &path, SourceFileType type) {
	// std::cout << "dep: " << path << '\n';
	switch (type)
	{
	case SourceFileType::C:
	case SourceFileType::CPP:
		return path.is_file();
	default:
		return path.is_file();
	}
}
