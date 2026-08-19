#include "ECSBatchGenerator.h"
#include "ECSGenerator.h"
#include "ECSParser.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
using namespace std;
namespace fs = std::filesystem;

bool ECSBatchGenerator::generateDirectory(const string& rootDirectory, ECSBatchResult& result, string& error)
{
	result = {};
	error.clear();
	fs::path rootPath(rootDirectory);
	error_code ec;
	rootPath = fs::absolute(rootPath, ec);
	if (ec || !fs::exists(rootPath) || !fs::is_directory(rootPath))
	{
		error = "Scan directory does not exist:" + rootDirectory;
		return false;
	}
	vector<fs::path> sourceFiles;
	fs::recursive_directory_iterator iter(rootPath, fs::directory_options::skip_permission_denied, ec);
	fs::recursive_directory_iterator end;
	for (; !ec && iter != end; iter.increment(ec))
	{
		if (iter->is_directory())
		{
			string name = iter->path().filename().string();
			if (name == ".git" || name == ".vs" || name == "Bin" || name == "Intermediate" || name == "build" || name == "build_vs2022")
			{
				iter.disable_recursion_pending();
			}
			continue;
		}
		if (!iter->is_regular_file())
		{
			continue;
		}
		string fileName = iter->path().filename().string();
		if (isSourceHeader(fileName) && !isGeneratedHeader(fileName))
		{
			sourceFiles.push_back(iter->path());
		}
	}
	if (ec)
	{
		error = "Failed to scan directory:" + rootDirectory + ", " + ec.message();
		return false;
	}
	sort(sourceFiles.begin(), sourceFiles.end(), [](const fs::path& a, const fs::path& b) { return a.generic_string() < b.generic_string(); });
	result.mScannedHeaderCount = static_cast<int>(sourceFiles.size());
	ECSParser parser;
	ECSGenerator generator;
	struct TypeSourceInfo
	{
		string mPath;
		int mLine = 0;
	};
	unordered_map<string, TypeSourceInfo> typeSourceMap;
	unordered_set<string> generatedPathSet;
	vector<string> generatedHeaderIncludes;
	vector<string> generatedCppIncludes;
	for (const fs::path& sourcePath : sourceFiles)
	{
		vector<ECSStructInfo> structList;
		if (!parser.parseFile(sourcePath.string(), structList, error))
		{
			return false;
		}
		string outputHeaderPath = makeGeneratedHeaderPath(sourcePath.string());
		string outputCppPath = makeGeneratedCppPath(sourcePath.string());
		if (structList.empty())
		{
			fs::remove(outputHeaderPath, ec);
			ec.clear();
			fs::remove(outputCppPath, ec);
			ec.clear();
			continue;
		}
		for (const ECSStructInfo& info : structList)
		{
			string qualifiedName = makeQualifiedName(info);
			auto found = typeSourceMap.find(qualifiedName);
			if (found != typeSourceMap.end())
			{
				error = sourcePath.string() + "(" + to_string(info.mLine) + "): EasyECS error: Duplicate ECS struct " + qualifiedName +
					"; first defined at " + found->second.mPath + "(" + to_string(found->second.mLine) + ")";
				return false;
			}
			typeSourceMap.emplace(qualifiedName, TypeSourceInfo{ sourcePath.string(), info.mLine });
		}
		string normalizedHeader = fs::weakly_canonical(fs::path(outputHeaderPath), ec).generic_string();
		if (ec)
		{
			ec.clear();
			normalizedHeader = fs::absolute(outputHeaderPath).generic_string();
		}
		if (!generatedPathSet.insert(normalizedHeader).second)
		{
			error = "Generated file name collision:" + outputHeaderPath;
			return false;
		}
		if (!generator.generate(structList, sourcePath.filename().string(), outputHeaderPath, outputCppPath, error))
		{
			error = sourcePath.string() + ":" + error;
			return false;
		}
		++result.mECSHeaderCount;
		result.mStructCount += static_cast<int>(structList.size());
		result.mFiles.push_back({ sourcePath.string(), static_cast<int>(structList.size()) });
		generatedHeaderIncludes.push_back(makeRelativeInclude(rootPath.string(), outputHeaderPath));
		generatedCppIncludes.push_back(makeRelativeInclude(rootPath.string(), outputCppPath));
	}
	unordered_set<string> expectedGeneratedFiles;
	for (const string& includePath : generatedHeaderIncludes)
	{
		expectedGeneratedFiles.insert(fs::absolute(rootPath / fs::path(includePath)).lexically_normal().generic_string());
	}
	for (const string& includePath : generatedCppIncludes)
	{
		expectedGeneratedFiles.insert(fs::absolute(rootPath / fs::path(includePath)).lexically_normal().generic_string());
	}
	fs::recursive_directory_iterator cleanIter(rootPath, fs::directory_options::skip_permission_denied, ec);
	for (; !ec && cleanIter != end; cleanIter.increment(ec))
	{
		if (!cleanIter->is_regular_file())
		{
			continue;
		}
		string fileName = cleanIter->path().filename().string();
		if (!hasSuffix(fileName, ".easyecs.generated.h") && !hasSuffix(fileName, ".easyecs.generated.cpp"))
		{
			continue;
		}
		string normalized = fs::absolute(cleanIter->path()).lexically_normal().generic_string();
		if (expectedGeneratedFiles.find(normalized) == expectedGeneratedFiles.end())
		{
			fs::remove(cleanIter->path(), ec);
		}
		ec.clear();
	}
	sort(generatedHeaderIncludes.begin(), generatedHeaderIncludes.end());
	sort(generatedCppIncludes.begin(), generatedCppIncludes.end());
	string aggregateHeader = "#pragma once\n";
	for (const string& includePath : generatedHeaderIncludes)
	{
		aggregateHeader += "#include \"" + includePath + "\"\n";
	}
	string aggregateCpp = "#include \"EasyECS.generated.h\"\n";
	for (const string& includePath : generatedCppIncludes)
	{
		aggregateCpp += "#include \"" + includePath + "\"\n";
	}
	if (!writeFileIfChanged((rootPath / "EasyECS.generated.h").string(), aggregateHeader, error))
	{
		return false;
	}
	return true;
}
bool ECSBatchGenerator::writeFileIfChanged(const string& filePath, const string& content, string& error)
{
	ifstream input(filePath, ios::binary);
	if (input.is_open())
	{
		stringstream buffer;
		buffer << input.rdbuf();
		if (buffer.str() == content)
		{
			return true;
		}
	}
	ofstream output(filePath, ios::binary | ios::trunc);
	if (!output.is_open())
	{
		error = "Cannot write file:" + filePath;
		return false;
	}
	output.write(content.data(), static_cast<streamsize>(content.size()));
	return output.good();
}
bool ECSBatchGenerator::isSourceHeader(const string& fileName)
{
	fs::path path(fileName);
	string extension = path.extension().string();
	transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
	return extension == ".h" || extension == ".hpp";
}
bool ECSBatchGenerator::isGeneratedHeader(const string& fileName)
{
	return fileName == "EasyECS.generated.h" || hasSuffix(fileName, ".easyecs.generated.h");
}
bool ECSBatchGenerator::hasSuffix(const string& value, const string& suffix)
{
	return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}
string ECSBatchGenerator::makeGeneratedHeaderPath(const string& sourcePath)
{
	fs::path path(sourcePath);
	return (path.parent_path() / (path.stem().string() + ".easyecs.generated.h")).string();
}
string ECSBatchGenerator::makeGeneratedCppPath(const string& sourcePath)
{
	fs::path path(sourcePath);
	return (path.parent_path() / (path.stem().string() + ".easyecs.generated.cpp")).string();
}
string ECSBatchGenerator::makeQualifiedName(const ECSStructInfo& info)
{
	string value;
	for (const string& item : info.mNamespaceList)
	{
		if (!value.empty())
		{
			value += "::";
		}
		value += item;
	}
	if (!value.empty())
	{
		value += "::";
	}
	value += info.mName;
	return value;
}
string ECSBatchGenerator::makeRelativeInclude(const string& rootDirectory, const string& filePath)
{
	error_code ec;
	fs::path relative = fs::relative(fs::path(filePath), fs::path(rootDirectory), ec);
	if (ec)
	{
		relative = fs::path(filePath).filename();
	}
	return relative.generic_string();
}
