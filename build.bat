@echo off
setlocal
set ARCH=%VSCMD_ARG_TGT_ARCH%
IF "%ARCH%"=="" goto vcerror
set DEBUG=0
set CODE_ROOT=
set COMPILER=cl

set ZLIB_ADD=
set ZLIB_DIR=.\zlib
set ZLIB_MODE=ReleaseWithoutAsm

set MSVC_OPTS=/MP /Gm-
set MSVC_LINKER=
set OBJDIR=objs
set MSVC_LIBS=ws2_32.lib zlibwapi.lib kernel32.lib dbghelp.lib

:argloop
IF "%1"=="" goto continue
IF "%1"=="cls" cls
IF "%1"=="cloc" goto :cloc
IF "%1"=="zdbg" set ZLIB_MODE=Debug
IF "%1"=="debug" set DEBUG=1
IF "%1"=="dbg" set DEBUG=1
IF "%1"=="run" set RUNMODE=0
IF "%1"=="onerun" set RUNMODE=1
IF "%1"=="clean" goto clean
IF "%1"=="2" set MSVC_OPTS=%MSVC_OPTS% /O2
IF "%1"=="1" set MSVC_OPTS=%MSVC_OPTS% /O1
IF "%1"=="0" set MSVC_OPTS=%MSVC_OPTS% /Od
IF "%1"=="wall" set MSVC_OPTS=%MSVC_OPTS% /Wall
IF "%1"=="w4" set MSVC_OPTS=%MSVC_OPTS% /W4
IF "%1"=="w0" set MSVC_OPTS=%MSVC_OPTS% /W0
IF "%1"=="wx" set MSVC_OPTS=%MSVC_OPTS% /WX
IF "%1"=="pb" goto pluginbuild
IF "%1"=="pluginbuild" goto pluginbuild
SHIFT
goto argloop

:pluginbuild
set BUILD_PLUGIN=1
SHIFT

set PLUGNAME=%1
SHIFT

IF "%1"=="install" (
	set PLUGINSTALL=1
	SHIFT
)
IF NOT "%1"=="" goto libloop

:libloop
IF "%1"=="" goto continue
set MSVC_LIBS=%MSVC_LIBS% %1
SHIFT
goto libloop

:continue
IF "%BUILD_PLUGIN%"=="1" (
  set COMPILER=cl /LD
  set OUTDIR=%PLUGNAME%\out\%ARCH%
  set BINNAME=%PLUGNAME%
  set OBJDIR=%PLUGNAME%\objs
  set CODE_ROOT=%PLUGNAME%\
) else (
  set BINNAME=server
  set OUTDIR=out\%ARCH%
)

set BINPATH=%OUTDIR%\%BINNAME%
set SERVER_ZDLL=%OUTDIR%\zlibwapi.dll

IF NOT EXIST %OBJDIR% MD %OBJDIR%
IF NOT EXIST %OUTDIR% MD %OUTDIR%

echo Build configuration:
echo Architecture: %ARCH%
IF "%DEBUG%"=="0" (echo Debug: disabled) else (
  set MSVC_OPTS=%MSVC_OPTS% /Z7
  set MSVC_LINKER=%MSVC_LINKER% /INCREMENTAL:NO /DEBUG /OPT:REF
  echo Debug: enabled
)

set ZLIB_COMPILEDIR=%ZLIB_DIR%\contrib\vstudio\vc14\%ARCH%\ZlibDll%ZLIB_MODE%
set ZLIB_DLL=%ZLIB_COMPILEDIR%\zlibwapi.dll

:msvc
IF "%BUILD_PLUGIN%"=="1" (
  set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH% /DCPLUGIN /Iheaders\
  set MSVC_LINKER=%MSVC_LINKER% /LIBPATH:out\%ARCH% /NOENTRY
  set MSVC_LIBS=%MSVC_LIBS% server.lib
) else (
set COPY=%ZLIB_DLL%
  set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH%
  copy /Y %ZLIB_DLL% %SERVER_ZDLL% 2> nul > nul
  IF NOT EXIST %SERVER_ZDLL% goto copyerror
)
set MSVC_OPTS=%MSVC_OPTS% /Fo%OBJDIR%\
set MSVC_OPTS=%MSVC_OPTS% /link /LIBPATH:%ZLIB_COMPILEDIR% %MSVC_LINKER%

%COMPILER% %CODE_ROOT%code\*.c /I%CODE_ROOT%headers /I%ZLIB_DIR% /I%ZLIB_DIR%\contrib %MSVC_OPTS% %MSVC_LIBS%
IF "%BUILD_PLUGIN%"=="1" (
	IF "%PLUGINSTALL%"=="1" (
		IF "%DEBUG%"=="1" (
			copy /y %OUTDIR%\%BINNAME%.* out\%ARCH%\plugins\
		) else (
			copy /y %OUTDIR%\%BINNAME%.dll out\%ARCH%\plugins\
		)
	)
  goto :end
) else (
  IF "%ERRORLEVEL%"=="0" (goto binstart) else (goto compileerror)
)

:copyerror
echo %COPY% not found
goto end

:compileerror
echo Something went wrong :(
goto end

:binstart
IF "%RUNMODE%"=="0" start /D %ARCH% %BINNAME%
IF "%RUNMODE%"=="1" goto onerun
goto end

:onerun:
pushd %OUTDIR%
%BINNAME%
popd
goto end

:cloc
cloc --exclude-dir=zlib .
goto end

:vcerror
echo Error: Script must be runned from VS Native Command Prompt.
echo Note: Also you can call "vcvars64" or "vcvars32" to configure VS Env.
goto end

:clean
del %OBJDIR%\*.obj %OUTDIR%\*.exe %OUTDIR%\*.dll

:end
endlocal
exit /B 0
