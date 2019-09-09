@echo off
set ARCH=x86
set DEBUG=0

set ZLIB_ADD=
set ZLIB_DIR=.\zlib
set ZLIB_MODE=Release

set LUA_ENABLED=0
set LUA_DIR=.\lua
set LUA_VER=51

set MSVC_OPTS=
set MSVC_LINKER=
set MSVC_OBJDIR=objs
set MSVC_LIBS=ws2_32.lib zlibwapi.lib

:argloop
IF "%1"=="" goto continue
IF "%1"=="cls" cls
IF "%1"=="clear" cls
IF "%1"=="cloc" goto :cloc
IF "%1"=="64" set ARCH=x64
IF "%1"=="noasm" set ZLIB_ADD=WithoutAsm
IF "%1"=="zdebug" set ZLIB_MODE=Debug
IF "%1"=="zdbg" set ZLIB_MODE=Debug
IF "%1"=="debug" set DEBUG=1
IF "%1"=="dbg" set DEBUG=1
IF "%1"=="run" set RUNMODE=0
IF "%1"=="onerun" set RUNMODE=1
IF "%1"=="clean" goto clean
IF "%1"=="2" set MSVC_OPTS=%MSVC_OPTS% /O2
IF "%1"=="1" set MSVC_OPTS=%MSVC_OPTS% /O1
IF "%1"=="0" set MSVC_OPTS=%MSVC_OPTS% /Od
IF "%1"=="allwarn" set MSVC_OPTS=%MSVC_OPTS% /Wall
IF "%1"=="w4" set MSVC_OPTS=%MSVC_OPTS% /W4
IF "%1"=="nowarn" set MSVC_OPTS=%MSVC_OPTS% /W0
IF "%1"=="wx" set MSVC_OPTS=%MSVC_OPTS% /WX
IF "%1"=="lua" set LUA_ENABLED=1
IF "%1"=="cp" set MSVC_OPTS=%MSVC_OPTS% /DCP_ENABLED
SHIFT
goto argloop
:continue

set EXECNAME=server
set EXECPATH=%ARCH%\%EXECNAME%
set SERVER_ZDLL=%ARCH%\zlibwapi.dll
set SERVER_LDLL=%ARCH%\lua%LUA_VER%.dll

IF NOT EXIST %MSVC_OBJDIR% MD %MSVC_OBJDIR%
IF NOT EXIST %ARCH% MD %ARCH%

echo Build configuration:
echo Architecture: %ARCH%
IF "%LUA_ENABLED%"=="0" (
  set MSVC_OPTS=/wd4206
  echo LuaPlugin: disabled
) else (
  set MSVC_OPTS=%MSVC_OPTS% /DLUA_ENABLED /I%LUA_DIR%\include
  set MSVC_LINKER=%MSVC_LINKER% /LIBPATH:%LUA_DIR%\lib_%ARCH%
  set MSVC_LIBS=%MSVC_LIBS% lua%LUA_VER%.lib
  set LUA_DLL=%LUA_DIR%\lib_%ARCH%\lua%LUA_VER%.dll
  echo LuaPlugin: enabled
)
IF "%DEBUG%"=="0" (
  echo Debug: disabled
) else (
  set MSVC_OPTS=%MSVC_OPTS% /Z7
  set MSVC_LINKER=%MSVC_LINKER% /INCREMENTAL:NO /DEBUG /OPT:REF
  echo Debug: enabled
)
IF "%ZLIB_ADD%"=="WithoutAsm" (echo Assembler: disabled) else (
  echo Assembler: enabled
  echo WARNINING: zlib assembler code may have bugs -- use at your own risk
  ping -n 4 127.0.0.1 > nul 2> nul
  if NOT "%ERRORLEVEL%"=="0" goto end
)

set ZLIB_COMPILEDIR=%ZLIB_DIR%\contrib\vstudio\vc14\%ARCH%\ZlibDll%ZLIB_MODE%%ZLIB_ADD%
set ZLIB_DLL=%ZLIB_COMPILEDIR%\zlibwapi.dll

:msvc
setlocal
set MSVC_OPTS=%MSVC_OPTS% /Fo%MSVC_OBJDIR%\ /Fe%EXECPATH%
set MSVC_OPTS=%MSVC_OPTS% /link /LIBPATH:%ZLIB_COMPILEDIR% %MSVC_LINKER%
set COPY=%ZLIB_DLL%

copy /Y %ZLIB_DLL% %SERVER_ZDLL% 2> nul > nul
IF NOT EXIST %SERVER_ZDLL% goto copyerror

IF "%LUA_ENABLED%"=="1" (
  set COPY=%LUA_DLL%
  copy /Y %LUA_DLL% %SERVER_LDLL% 2> nul > nul
  IF NOT EXIST %SERVER_LDLL% goto copyerror
)

IF "%ARCH%"=="x64" call vcvars64
IF "%ARCH%"=="x86" call vcvars32
cl .\code\*.c /MP /W4 /Gm- /I.\headers\ /I%ZLIB_DIR% %MSVC_LIBS% %MSVC_OPTS%
IF "%ERRORLEVEL%"=="0" (goto binstart) else (goto compileerror)

:copyerror
echo %COPY% not found
endlocal
goto end

:compileerror
endlocal
echo Something went wrong :(
goto end

:binstart
IF "%RUNMODE%"=="0" start /D %ARCH% %EXECNAME%
IF "%RUNMODE%"=="1" goto onerun
endlocal
goto end

:onerun:
pushd %ARCH%
%EXECNAME%
popd
goto end

:cloc
cloc --exclude-dir=zlib,lua .
goto end

:clean
del objs\*.obj *.exe *.dll

:end
exit /B 0
