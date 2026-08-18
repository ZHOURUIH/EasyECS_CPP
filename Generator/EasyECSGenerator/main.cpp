#include "ECSBatchGenerator.h"
#include "ECSGenerator.h"
#include "ECSParser.h"
#include "GeneratorUtility.h"
#include <cstdio>
#include <string>
#include <vector>
using namespace std;

static string getFileName(const string& path)
{
	size_t pos = path.find_last_of("/\\");
	return pos == string::npos ? path : path.substr(pos + 1);
}
static string getQualifiedName(const ECSStructInfo& info)
{
	string value;
	for (const string& item : info.mNamespaceList)
	{
		if (!value.empty()) value += "::";
		value += item;
	}
	if (!value.empty()) value += "::";
	value += info.mName;
	return value;
}
static int runSingleFile(const vector<string>& arguments, bool pause)
{
	if (arguments.size() != 3)
	{
		printf("Single file usage:\nEasyECSGenerator [--no-pause] <input.h> <output.generated.h> <output.generated.cpp>\n");
		return EasyECSGeneratorUtility::finishProgram(1, pause);
	}
	string inputPath = arguments[0];
	string outputHeaderPath = arguments[1];
	string outputCppPath = arguments[2];
	ECSParser parser;
	vector<ECSStructInfo> structList;
	string error;
	if (!parser.parseFile(inputPath, structList, error))
	{
		printf("EasyECS parse failed:%s\n", error.c_str());
		return EasyECSGeneratorUtility::finishProgram(2, pause);
	}
	if (structList.empty())
	{
		printf("EasyECS parse failed:No ECS() struct found in file:%s\n", inputPath.c_str());
		return EasyECSGeneratorUtility::finishProgram(2, pause);
	}
	ECSGenerator generator;
	if (!generator.generate(structList, getFileName(inputPath), outputHeaderPath, outputCppPath, error))
	{
		printf("EasyECS generate failed:%s\n", error.c_str());
		return EasyECSGeneratorUtility::finishProgram(3, pause);
	}
	printf("EasyECS generate success. StructCount:%zu\n", structList.size());
	for (const ECSStructInfo& info : structList) printf("  %s FieldCount:%zu\n", getQualifiedName(info).c_str(), info.mFields.size());
	return EasyECSGeneratorUtility::finishProgram(0, pause);
}
static int runDirectoryScan(const vector<string>& arguments, bool pause)
{
	if (arguments.size() != 2 || arguments[0] != "--scan")
	{
		printf("Directory scan usage:\nEasyECSGenerator [--no-pause] --scan <directory>\n");
		return EasyECSGeneratorUtility::finishProgram(1, pause);
	}
	ECSBatchGenerator generator;
	ECSBatchResult result;
	string error;
	if (!generator.generateDirectory(arguments[1], result, error))
	{
		printf("EasyECS batch generate failed:%s\n", error.c_str());
		return EasyECSGeneratorUtility::finishProgram(4, pause);
	}
	printf("EasyECS batch generate success. ScannedHeaders:%d ECSHeaders:%d StructCount:%d\n",
		result.mScannedHeaderCount, result.mECSHeaderCount, result.mStructCount);
	for (const ECSBatchFileResult& file : result.mFiles) printf("  %s StructCount:%d\n", file.mSourcePath.c_str(), file.mStructCount);
	return EasyECSGeneratorUtility::finishProgram(0, pause);
}
int main(int argc, char** argv)
{
	EasyECSGeneratorUtility::initConsole();
	bool pause = !EasyECSGeneratorUtility::hasArgument(argc, argv, "--no-pause");
	vector<string> arguments;
	for (int i = 1; i < argc; ++i)
	{
		if (string(argv[i]) != "--no-pause") arguments.push_back(argv[i]);
	}
	if (!arguments.empty() && arguments[0] == "--scan") return runDirectoryScan(arguments, pause);
	return runSingleFile(arguments, pause);
}
