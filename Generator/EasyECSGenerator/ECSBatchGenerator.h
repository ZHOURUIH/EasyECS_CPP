#pragma once
#include <string>
#include <vector>

struct ECSBatchFileResult
{
	std::string mSourcePath;
	int mStructCount = 0;
};
struct ECSBatchResult
{
	int mScannedHeaderCount = 0;
	int mECSHeaderCount = 0;
	int mStructCount = 0;
	std::vector<ECSBatchFileResult> mFiles;
};
class ECSBatchGenerator
{
public:
	bool generateDirectory(const std::string& rootDirectory, ECSBatchResult& result, std::string& error);
private:
	bool writeFileIfChanged(const std::string& filePath, const std::string& content, std::string& error);
	bool isSourceHeader(const std::string& fileName);
	bool isGeneratedHeader(const std::string& fileName);
	bool hasSuffix(const std::string& value, const std::string& suffix);
	std::string makeGeneratedHeaderPath(const std::string& sourcePath);
	std::string makeGeneratedCppPath(const std::string& sourcePath);
	std::string makeQualifiedName(const struct ECSStructInfo& info);
	std::string makeRelativeInclude(const std::string& rootDirectory, const std::string& filePath);
};
