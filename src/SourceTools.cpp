#include "SourceTools.hpp"
#include "FilePath.hpp"
#include "code/CPreprocessor.hpp"



vector<string> SourceTools::get_dependencies(const StrBlob &file, SourceType type) {
	switch (type)
	{
	case SourceType::C:
		// case SourceType::CPP: // equal to SourceType::C
		{
			vector<CPreprocessor::Token> tokens{};

			CPreprocessor::gather_all_tks(file, tokens);

			vector<string> results{};

			for (const auto &token : tokens)
			{
				if (token.type == CPreprocessor::Type::Include)
				{
					results.push_back(
						CPreprocessor::get_include_path(file.slice(token.value.begin, token.value.end))
					);
				}
			}

			return
		}
	}
	return;
}

