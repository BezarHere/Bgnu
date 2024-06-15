#include "SourceTools.hpp"
#include "FilePath.hpp"
#include "code/CPreprocessor.hpp"



void SourceTools::get_dependencies(const StrBlob &file, SourceFileType type,
																	 vector<string> &out) {
	switch (type)
	{
	case SourceFileType::C:
	case SourceFileType::CPP:
		{
			vector<CPreprocessor::Token> tokens{};

			CPreprocessor::gather_all_tks(file, tokens);


			for (const auto &token : tokens)
			{
				if (token.type == CPreprocessor::Type::Include)
				{
					out.push_back(
						CPreprocessor::get_include_path(file.slice(token.value.begin, token.value.end))
					);
				}
			}

			return;
		}

	default:
		Logger::error("get_dependencies() of type %d is not implemented", (int)type);
		return;
	}
}

void SourceProcessor::add_file(const FilePath &filepath) {
	auto data = filepath.read_string();
	return add_file(StrBlob(data.data(), data.length()));
}

void SourceProcessor::add_file(const StrBlob &source) {



}

void SourceProcessor::process() {
}

hash_t SourceProcessor::get_file_hash(const FilePath &filepath) const {
	return m_info_map.at(filepath).hash;
}

bool SourceProcessor::is_processing_path(const FilePath &filepath) const {
	for (const auto &fp : m_in_loading)
	{
		if (fp == filepath)
		{
			return true;
		}
	}
	return false;
}

void SourceProcessor::process_dependencies(const FilePath &filepath) {
	const DependencyInfo &dep_info = m_info_map.at(filepath);


}
