#include "BuildTools.hpp"
#include "utility/Process.hpp"
#include "utility/ThreadBatcher.hpp"


using namespace build_tools;

static bool DoesSourceTypeDominate(SourceFileType type, SourceFileType target);
static void GenerateJoinArgs(const Blob<const ExecuteParameter> &params, std::string *results);


void build_tools::DeleteBuildCache(const Project &project) {
	project.get_output().cache_dir->remove_recursive();
}

void build_tools::DeleteBuildDir(const Project &project) {
	DeleteBuildCache(project);
	project.get_output().dir->remove_recursive();
}

void build_tools::SetupHashes(BuildCache &cache, const Project &proj, const BuildConfiguration *config) {
	cache.build_hash = proj.hash();
	cache.config_hash = config->hash();
}

void build_tools::DeleteUnusedObjFiles(const std::set<FilePath> &object_files, const std::set<FilePath> &used_files) {
	for (const auto &obj : object_files)
	{
		if (used_files.contains(obj))
		{
			continue;
		}

		if (obj.is_file())
		{
			obj.remove();
		}
	}
}



std::vector<int> build_tools::Execute(const Blob<const ExecuteParameter> &params) {

	std::vector<std::string> joined_commands{};
	joined_commands.resize(params.length());

	GenerateJoinArgs(
		params,
		joined_commands.data()
	);

	vector<int> results{};
	results.resize(params.length());

	const errno_t error = _Execute_Inner(
		{joined_commands.data(), joined_commands.size()},
		params,
		{results.data(), results.size()}
	);

	return results;
}

std::vector<int> build_tools::Execute_Multithreaded(const Blob<const ExecuteParameter> &params, uint32_t batches) {

	std::vector<std::string> joined_commands{};
	joined_commands.resize(params.length());

	GenerateJoinArgs(
		params,
		joined_commands.data()
	);

	vector<int> results{};
	results.resize(params.length());

	const errno_t error = _ExecuteParallel_Inner(
		{joined_commands.data(), joined_commands.size()},
		params,
		{results.data(), results.size()},
		batches
	);

	return results;
}

errno_t build_tools::_Execute_Inner(const Blob<const std::string> &args, const Blob<const ExecuteParameter> &params, Blob<int> results) {
	for (size_t i = 0; i < args.size; i++)
	{
		const auto &param = params[i];
		Process process{args[i]};
		process.set_name(param.name.c_str());

		if (HAS_FLAG(param.flags, eExcFlag_Printout))
		{
			Logger::notify("executing '%s'...", param.name.c_str());
		}

		results[i] = process.start(param.out);

		if (HAS_FLAG(param.flags, eExcFlag_Printout))
		{
			Logger::notify(
				"executing '%s' resulted in code %d [%llu / %llu]",
				param.name.c_str(), results[i], i, args.size
			);
		}
	}

	return EOK;
}

errno_t build_tools::_ExecuteParallel_Inner(const Blob<const std::string> &args, const Blob<const ExecuteParameter> &params, Blob<int> results, uint32_t batches) {
	std::atomic_size_t progress_index = 0;
	const auto func = [&args, &params, &progress_index](size_t index) {
		const auto &param = params[index];
		Process process{args[index]};
		process.set_name(param.name.c_str());

		Logger::verbose("processing parallel: '%s'", args[index].c_str());

		if (HAS_FLAG(param.flags, eExcFlag_Printout))
		{
			Logger::notify("executing '%s'...", param.name.c_str());
		}

		const int result = process.start(param.out);
		progress_index++;

		if (HAS_FLAG(param.flags, eExcFlag_Printout))
		{
			Logger::notify(
				"executing '%s' resulted in %s [%d] [%llu / %llu]",
				param.name.c_str(), GetErrorName(result), result, (size_t)progress_index, args.size
			);
		}

		return result;
		};

	const auto exporter = [&results](size_t index, int result) {
		results[index] = result;
		};

	ThreadBatcher batcher = {
		batches,
		func,
		exporter
	};

	batcher.run(args.size);

	return errno_t();
}

SourceFileType build_tools::GetDominantSourceType(Blob<const SourceFileType> file_types) {
	SourceFileType dominate_type = SourceFileType::None;

	for (SourceFileType type : file_types)
	{
		if (DoesSourceTypeDominate(type, dominate_type))
		{
			dominate_type = type;
		}
	}

	return dominate_type;
}

SourceFileType build_tools::DefaultSourceFileTypeForFilePath(const FilePath::char_type *path) {
	const size_t extension_index = string_tools::find_last(
		path, '.', FilePath::MaxPathLength
	);
	const size_t path_length = string_tools::length(path);

	if (extension_index == npos || extension_index + 1 >= path_length)
	{
		return SourceFileType::None;
	}

	const auto *extension = path + extension_index + 1;

	return DefaultSourceFileTypeForExtension(
		FilePath::string_blob{extension, path_length - extension_index - 1}
	);
}

SourceFileType build_tools::DefaultSourceFileTypeForFilePath(const FilePath &path) {
	return DefaultSourceFileTypeForFilePath(path.c_str());
}

SourceFileType build_tools::DefaultSourceFileTypeForExtension(FilePath::string_blob extension) {
	constexpr const FilePath::char_type *CPPExtensions[] = {
		"cpp",
		"cc",
		"cxx",

		"hpp",
		"hh",
		"hxx",
	};

	constexpr const FilePath::char_type *CExtensions[] = {
		"c",
		"h",
	};

#define CHECK_EXT(type, ext_list) for (size_t i = 0; i < std::size(ext_list); i++) \
{if (string_tools::equal_insensitive(extension.data, (ext_list)[i])) { return type; } }

	CHECK_EXT(SourceFileType::CPP, CPPExtensions);
	CHECK_EXT(SourceFileType::C, CExtensions);

	return SourceFileType::None;
}

SourceFileType build_tools::DefaultSourceFileTypeForExtension(const FilePath::string_type &extension) {
	return DefaultSourceFileTypeForExtension(
		FilePath::string_blob{extension.c_str(), extension.length()}
	);
}

const char *build_tools::GetSourceFileTypeName(SourceFileType type) {
	switch (type)
	{
	case SourceFileType::C:
		return "C";
	case SourceFileType::CPP:
		return "Cpp";
	default:
		return "unknown";
	}
}

bool DoesSourceTypeDominate(SourceFileType type, SourceFileType target) {
	constexpr std::pair<SourceFileType, SourceFileType> DominationTable[] = {
		{SourceFileType::CPP, SourceFileType::C},
	};

	// a type can not dominate it's own kind
	if (type == target)
	{
		return false;
	}

	// every type dominates the none type
	if (target == SourceFileType::None)
	{
		return true;
	}

	// the none type can't dominate any type
	if (type == SourceFileType::None)
	{
		return false;
	}

	for (size_t i = 0; i < std::size(DominationTable); i++)
	{
		if (DominationTable[i] == decltype(DominationTable[0]){type, target})
		{
			return true;
		}
	}

	return false;
}

void GenerateJoinArgs(const Blob<const ExecuteParameter> &params, std::string *results) {

	for (size_t i = 0; i < params.length(); i++)
	{
		std::ostringstream oss;

		for (const string &str : params[i].args)
		{
			oss << str << ' ';
		}

		results[i] = oss.str();
		results[i].resize(results[i].size() - 1);
	}
}
