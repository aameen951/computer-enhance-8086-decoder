@ECHO OFF
SETLOCAL

@SET PROJECT_DIR=%~dp0
@IF "%PROJECT_DIR:~-1%" EQU "\" SET PROJECT_DIR=%PROJECT_DIR:~0,-1%

SET BUILD_DIR=%PROJECT_DIR%\build

if not exist "%BUILD_DIR%" MKDIR "%BUILD_DIR%"

pushd "%BUILD_DIR%"

nasm "%PROJECT_DIR%\%1" -o output.bin
IF ERRORLEVEL 1 goto :error

main.exe output.bin > disassembled.asm
IF ERRORLEVEL 1 goto :error

nasm disassembled.asm -o disassembled.bin
IF ERRORLEVEL 1 goto :error

checksum output.bin disassembled.bin
IF ERRORLEVEL 1 goto :error

popd

echo.
echo OK
ENDLOCAL
EXIT /B 0
:error
ENDLOCAL
EXIT /B 1
