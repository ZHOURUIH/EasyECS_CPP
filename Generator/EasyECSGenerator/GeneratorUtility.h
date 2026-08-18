#pragma once
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#undef max
#undef min
#endif

namespace EasyECSGeneratorUtility
{
inline void initConsole()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif
}
inline bool hasArgument(int argc, char** argv, const char* argument)
{
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], argument) == 0) return true;
	}
	return false;
}
inline void pauseConsole()
{
	std::printf("\nPress Enter to exit...");
	std::fflush(stdout);
	std::getchar();
}
inline int finishProgram(int returnCode, bool pause)
{
	if (pause) pauseConsole();
	return returnCode;
}
}
