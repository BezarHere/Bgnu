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

FilePath SourceTools::get_intermediate_filepath(const FilePath &filepath, const FilePath &dst_dir) {
	char buffer[BUFSIZ] = {};
	sprintf_s(buffer, "%s.%X", filepath.name().c_str(), gen_hash(filepath.get_text()));
	return dst_dir.join_path(buffer);
}


