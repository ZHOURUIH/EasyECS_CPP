@echo off
setlocal EnableExtensions
set "GENERATOR_RELEASE=%~dp0..\..\Generator\Bin\Release\EasyECSGenerator.exe"
set "GENERATOR_DEBUG=%~dp0..\..\Generator\Bin\Debug\EasyECSGenerator.exe"
set "GENERATOR="
if exist "%GENERATOR_RELEASE%" set "GENERATOR=%GENERATOR_RELEASE%"
if not defined GENERATOR if exist "%GENERATOR_DEBUG%" set "GENERATOR=%GENERATOR_DEBUG%"
if not defined GENERATOR (
	call :checkGenerated
	if not errorlevel 1 (
		echo [EasyECS] EasyECSGenerator.exe not found. Using checked-in generated test code.
		exit /b 0
	)
	echo [EasyECS] EasyECSGenerator.exe not found and generated test code is incomplete.
	echo [EasyECS] Please build Generator\EasyECSGenerator.sln first.
	exit /b 1
)
echo [EasyECS] Generator: %GENERATOR%
echo [EasyECS] Scan: %~dp0Data
"%GENERATOR%" --no-pause --scan "%~dp0Data"
if errorlevel 1 exit /b %errorlevel%
exit /b 0

:checkGenerated
for %%F in (
	"Data\EasyECS.generated.h"
	"Data\EasyECS.generated.cpp"
	"Data\RoleData.easyecs.generated.h"
	"Data\RoleData.easyecs.generated.cpp"
	"Data\CharacterData.easyecs.generated.h"
	"Data\CharacterData.easyecs.generated.cpp"
	"Data\ItemData.easyecs.generated.h"
	"Data\ItemData.easyecs.generated.cpp"
	"Data\Battle\BulletData.easyecs.generated.h"
	"Data\Battle\BulletData.easyecs.generated.cpp"
) do if not exist "%~dp0%%~F" exit /b 1
exit /b 0
