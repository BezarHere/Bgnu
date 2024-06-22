#include "SourceProcessor.hpp"


void SourceProcessor::add_file(const FilePath &filepath, const DependencyInfo &dep) {
	m_info_map.insert_or_assign(filepath, dep);
	m_info_map.at(filepath).hash = 0;
}

void SourceProcessor::process() {
}

hash_t SourceProcessor::get_file_hash(const FilePath &filepath) const {
	return m_info_map.at(filepath).hash;
}

bool SourceProcessor::_is_processing_path(const FilePath &filepath) const {
	for (const FilePath &path : m_loading_stack)
	{
		if (path == filepath)
		{
			return true;
		}
	}
	return false;
}

void SourceProcessor::_push_processing_path(const FilePath &filepath) {
	m_loading_stack.push_back(filepath);
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

	if (m_loading_stack.back() != filepath)
	{
		Logger::error(
			"Ill-loading stack: expected the top to be \"%s\", but it is \"%s\"",
			filepath.c_str(),
			m_loading_stack.back().c_str()
		);
		return;
	}

	m_loading_stack.pop_back();
}

void SourceProcessor::_process_dependencies(const FilePath &filepath) {
	const DependencyInfo &dep_info = m_info_map.at(filepath);

	_push_processing_path(filepath);

	for (size_t i = 0; i < dep_info.sub_dependencies.size(); i++)
	{
		FilePath dependency_path = _find_dependency(i, filepath, dep_info);

		if (!dependency_path.exists())
		{
			(has_flags(eFlag_WarnAbsentDependencies) ? Logger::warning : Logger::error)(
				"dependency named \"%s\" couldn't be found for file at \"%s\"",
				dep_info.sub_dependencies[i].c_str(),
				filepath.get_text().data
				);

			continue;
		}

		// this path is being processed, but given we encounter it here, there is an include cycle
		// skip processing (any compiler will abort compiling this)
		if (_is_processing_path(filepath))
		{
			continue;
		}

		DependencyInfo map;

		map.type = dep_info.type;
		map.


			add_file()
	}

	_pop_processing_path(filepath);
}

FilePath SourceProcessor::_find_dependency(const size_t name_index,
																					 const FilePath &file,
																					 const DependencyInfo &info) {
	const dependency_name &name = info.sub_dependencies[name_index];

	// is the dependency path local to the file?
	{
		FilePath local_path = FilePath(name, file);

		if (_dependency_exists(local_path, info.type))
		{
			return local_path;
		}
	}

	for (const auto &dir : info.included_directories)
	{
		FilePath dir_path = FilePath(name, dir);
		if (_dependency_exists(dir_path, info.type))
		{
			return dir_path;
		}
	}

	return {};
}

bool SourceProcessor::_dependency_exists(const FilePath &path, SourceFileType type) {
	switch (type)
	{
	case SourceFileType::C:
	case SourceFileType::CPP:
		return path.is_file();
	default:
		return path.is_file();
	}
}
