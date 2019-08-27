@echo off
cls

set OLDPATH=%PATH%

set ZLIB_DIR=.\zlib-1.2.11
set ZLIB_ARCH=x86
set ZLIB_MODE=Release
set ZLIB_ADD=
set ERROR=0

:argloop
IF "%1"=="" goto continue
IF "%1"=="gcc" goto gcc
IF "%1"=="64" set ZLIB_ARCH=x64
IF "%1"=="noasm" set ZLIB_ADD=WithoutAsm
IF "%1"=="debug" set ZLIB_MODE=Debug
IF "%1"=="dbg" set ZLIB_MODE=Debug
SHIFT
goto argloop
:continue

echo.
echo zlib configuration:
echo Architecture: %ZLIB_ARCH%
IF "%ZLIB_ADD%"=="WithoutAsm" (echo Assembler: disabled) else (echo Assembler: enabled)
echo.

set EXECNAME=%ZLIB_ARCH%-server
set ZLIB_COMPILEDIR=%ZLIB_DIR%\contrib\vstudio\vc14\%ZLIB_ARCH%\ZlibDll%ZLIB_MODE%%ZLIB_ADD%

:msvc
IF "%ZLIB_ARCH%"=="x64" call vcvars64
IF "%ZLIB_ARCH%"=="x86" call vcvars32
copy /Y %ZLIB_COMPILEDIR%\zlibwapi.dll .
IF NOT "%ERRORLEVEL%"=="0" goto copyerror
cl *.c /I%ZLIB_DIR% ws2_32.lib %ZLIB_COMPILEDIR%\zlibwapi.lib /Fe%EXECNAME%
IF NOT "%ERRORLEVEL%"=="0" set ERROR=1
goto end

:gcc
IF "%ZLIB_ARCH%"=="x64" gccvars64
IF "%ZLIB_ARCH%"=="x86" gccvars32
gcc *.c z%ZLIB_ARCH%.dll -lws2_32 -o %EXECNAME%
IF NOT "%ERRORLEVEL%"=="0" set ERROR=1
goto end

:end
set PATH=%OLDPATH%
IF "%ERROR%"=="0" (goto binstart) else (goto compileerror)

:copyerror
echo %ZLIB_COMPILEDIR%\zlibwapi.dll not found
exit /B 0

:compileerror
echo Something went wrong :(
exit /B 0

:binstart
start %EXECNAME%
