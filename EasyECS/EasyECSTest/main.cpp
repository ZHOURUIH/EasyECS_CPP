#include "EasyECS.h"
#include "GeneratorSmokeTest.h"
#include "RoleDataDictionaryBenchmark.h"
#include "RoleDataListBenchmark.h"
#include <cstdio>
#include <cstring>

static int runAllTests()
{
	int result = runGeneratorSmokeTest();
	if (result != 0) return result;
	result = runRoleDataListBenchmark();
	if (result != 0) return result;
	result = runRoleDataDictionaryBenchmark();
	if (result != 0) return result;
	return runRoleDataDictionaryFuzzTest();
}
static int runTestByName(const char* name)
{
	if (std::strcmp(name, "smoke") == 0) return runGeneratorSmokeTest();
	if (std::strcmp(name, "list") == 0) return runRoleDataListBenchmark();
	if (std::strcmp(name, "dictionary") == 0 || std::strcmp(name, "dict") == 0) return runRoleDataDictionaryBenchmark();
	if (std::strcmp(name, "fuzz") == 0) return runRoleDataDictionaryFuzzTest();
	if (std::strcmp(name, "all") == 0) return runAllTests();
	std::printf("Unknown test:%s\n", name);
	std::printf("Available: smoke | list | dictionary | fuzz | all\n");
	return 1;
}
static int showMenuAndRun()
{
	std::printf("================ EasyECS Test ================\n");
	std::printf("1. Generator Smoke Test\n");
	std::printf("2. RoleData List Benchmark\n");
	std::printf("3. RoleData Dictionary Benchmark\n");
	std::printf("4. Dictionary Fuzz Test\n");
	std::printf("5. Run All\n");
	std::printf("Select:");
	char input[32]{};
	if (std::fgets(input, sizeof(input), stdin) == nullptr) return 1;
	switch (input[0])
	{
	case '1': return runGeneratorSmokeTest();
	case '2': return runRoleDataListBenchmark();
	case '3': return runRoleDataDictionaryBenchmark();
	case '4': return runRoleDataDictionaryFuzzTest();
	case '5': return runAllTests();
	default:
		std::printf("Invalid selection.\n");
		return 1;
	}
}
int main(int argc, char** argv)
{
	EasyECSRuntime::initConsole();
	bool pause = !EasyECSRuntime::hasArgument(argc, argv, "--no-pause");
	const char* testName = nullptr;
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--no-pause") != 0)
		{
			testName = argv[i];
			break;
		}
	}
	int result = testName != nullptr ? runTestByName(testName) : showMenuAndRun();
	return EasyECSRuntime::finishProgram(result, pause);
}
