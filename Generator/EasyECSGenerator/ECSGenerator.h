#pragma once
#include "ECSParser.h"
#include <string>
#include <vector>

class ECSGenerator
{
public:
	bool generate(const std::vector<ECSStructInfo>& structList, const std::string& sourceHeaderName, const std::string& outputHeaderPath,
		const std::string& outputCppPath, std::string& error);
private:
	std::string makeColumnSuffix(const std::string& fieldName);
	void appendStructHeader(std::string& output, const ECSStructInfo& info);
	void appendStructCpp(std::string& output, const ECSStructInfo& info, const std::string& generatedHeaderName);
	void appendNamespaceBegin(std::string& output, const ECSStructInfo& info);
	void appendNamespaceEnd(std::string& output, const ECSStructInfo& info);
	std::string getNamespaceName(const ECSStructInfo& info);
	std::string getQualifiedName(const ECSStructInfo& info);
	std::string fileNameOnly(const std::string& path);
	bool writeFileIfChanged(const std::string& filePath, const std::string& content, std::string& error);
};
