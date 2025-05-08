@ECHO OFF
SETLOCAL

@SET PROJECT_DIR=%~dp0
@IF "%PROJECT_DIR:~-1%" EQU "\" SET PROJECT_DIR=%PROJECT_DIR:~0,-1%

SET BUILD_DIR=%PROJECT_DIR%\build

if not exist "%BUILD_DIR%" MKDIR "%BUILD_DIR%"

pushd "%BUILD_DIR%"

"%VC64_CL%" /nologo /Zi /O2 "%PROJECT_DIR%\gen.cpp"
"%VC64_CL%" /nologo /Zi /O2 "%PROJECT_DIR%\haversine.cpp" "%PROJECT_DIR%\json.cpp"

popd

ENDLOCAL
EXIT /B 0
