@echo off
cls

set MSVC_OPTS=
set ZLIB_DIR=.\zlib
set ZLIB_ARCH=x86
set ZLIB_MODE=Release
set ZLIB_ADD=
set ERROR=0
set NORUN=0

:argloop
IF "%1"=="" goto continue
IF "%1"=="cloc" goto :cloc
IF "%1"=="gcc" goto gcc
IF "%1"=="64" set ZLIB_ARCH=x64
IF "%1"=="noasm" set ZLIB_ADD=WithoutAsm
IF "%1"=="zdebug" set ZLIB_MODE=Debug
IF "%1"=="zdbg" set ZLIB_MODE=Debug
IF "%1"=="norun" set NORUN=1
IF "%1"=="onerun" set NORUN=2
IF "%1"=="clean" goto clean
IF "%1"=="2" set MSVC_OPTS=%MSVC_OPTS% /O2
IF "%1"=="1" set MSVC_OPTS=%MSVC_OPTS% /O1
IF "%1"=="noopt" set MSVC_OPTS=%MSVC_OPTS% /Od
IF "%1"=="allwarn" set MSVC_OPTS=%MSVC_OPTS% /Wall
IF "%1"=="w4" set MSVC_OPTS=%MSVC_OPTS% /W4
IF "%1"=="nowarn" set MSVC_OPTS=%MSVC_OPTS% /W0
IF "%1"=="wx" set MSVC_OPTS=%MSVC_OPTS% /WX
SHIFT
goto argloop
:continue

echo.
echo zlib configuration:
echo Architecture: %ZLIB_ARCH%
IF "%ZLIB_ADD%"=="WithoutAsm" (echo Assembler: disabled) else (
echo Assembler: enabled
echo WARNINING: zlib assembler code may have bugs -- use at your own risk
ping -n 4 127.0.0.1 > nul 2> nul
)
echo.

md %ZLIB_ARCH% > nul 2> nul
set EXECNAME=server
set ZLIB_COMPILEDIR=%ZLIB_DIR%\contrib\vstudio\vc14\%ZLIB_ARCH%\ZlibDll%ZLIB_MODE%%ZLIB_ADD%

:msvc
setlocal
IF "%ZLIB_ARCH%"=="x64" call vcvars64
IF "%ZLIB_ARCH%"=="x86" call vcvars32
copy /Y %ZLIB_COMPILEDIR%\zlibwapi.dll %ZLIB_ARCH% > nul
IF NOT "%ERRORLEVEL%"=="0" goto copyerror
cl *.c /MP /W4 /Gm- /I%ZLIB_DIR% ws2_32.lib %ZLIB_COMPILEDIR%\zlibwapi.lib /Fe%ZLIB_ARCH%\%EXECNAME% %MSVC_OPTS%
IF NOT "%ERRORLEVEL%"=="0" goto compileerror
goto end

:gcc
setlocal
IF "%ZLIB_ARCH%"=="x64" call gccvars64
IF "%ZLIB_ARCH%"=="x86" call gccvars32
gcc *.c z.dll -lws2_32 -o %ZLIB_ARCH%/%EXECNAME%
IF NOT "%ERRORLEVEL%"=="0" set ERROR=1
goto end

:end
IF "%ERROR%"=="0" (goto binstart) else (goto compileerror)

:copyerror
echo %ZLIB_COMPILEDIR%\zlibwapi.dll not found
endlocal
exit /B 0

:compileerror
endlocal
echo Something went wrong :(
exit /B 0

:binstart
IF "%NORUN%"=="0" start /D %ZLIB_ARCH% %EXECNAME%
IF "%NORUN%"=="2" goto onerun
endlocal
exit /B 0

:onerun:
pushd %ZLIB_ARCH%
%EXECNAME%
popd
exit /B 0

:cloc
cloc --exclude-dir=zlib .
exit /B 0

:clean
del *.obj *.exe *.dll
