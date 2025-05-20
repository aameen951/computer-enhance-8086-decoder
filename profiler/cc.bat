@ECHO OFF
SETLOCAL

@SET PROJECT_DIR=%~dp0
@IF "%PROJECT_DIR:~-1%" EQU "\" SET PROJECT_DIR=%PROJECT_DIR:~0,-1%

SET BUILD_DIR=%PROJECT_DIR%\build

if not exist "%BUILD_DIR%" MKDIR "%BUILD_DIR%"

pushd "%BUILD_DIR%"

"%VC64_CL%" /c /nologo /Zi /O2 "%PROJECT_DIR%\main.cpp"
"%VC64_CL%" /c /nologo /Zi /O2 /TP /D"PERF_PROFILER_IMPLEMENTATION" "%PROJECT_DIR%\..\shared\perf_profiler.h"

"%VC64_LINK%" /nologo /DEBUG /OUT:main.exe main.obj perf_profiler.obj 

popd

ENDLOCAL
EXIT /B 0
