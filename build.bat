@echo off
setlocal enableextensions enabledelayedexpansion

set ARCH=%VSCMD_ARG_TGT_ARCH%
set CLEAN=0
set DEBUG=0
set PROJECT_ROOT=.
set COMPILER=cl
set BUILD_PLUGIN=0

set SVOUTDIR=.\out\%ARCH%
set BINNAME=server.exe

set WARN_LEVEL=/W3
set OPT_LEVEL=/O2

set MSVC_LINKER=/INCREMENTAL:NO /OPT:REF /SUBSYSTEM:CONSOLE
set MSVC_OPTS=/MP /GL /Oi /Gy /fp:fast /DMINIZ_NO_STDIO /DMINIZ_NO_ARCHIVE_APIS /DMINIZ_NO_TIME
set OBJDIR=objs
set MSVC_LIBS=ws2_32.lib kernel32.lib dbghelp.lib advapi32.lib
FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --short HEAD`) DO (
	set MSVC_OPTS=%MSVC_OPTS% /DGIT_COMMIT_SHA#"\"%%F\""
)

:argloop
IF "%1"=="" goto continue
IF "%1"=="cls" cls
IF "%1"=="cloc" goto cloc
IF "%1"=="debug" set DEBUG=1
IF "%1"=="dbg" set DEBUG=1
IF "%1"=="run" set RUNMODE=0
IF "%1"=="onerun" set RUNMODE=1
IF "%1"=="clean" set CLEAN=1
IF "%1"=="o2" set OPT_LEVEL=/O2
IF "%1"=="o1" set OPT_LEVEL=/O1
IF "%1"=="o0" set OPT_LEVEL=/Od
IF "%1"=="wall" set WARN_LEVEL=/Wall
IF "%1"=="w4" set WARN_LEVEL=/W4
IF "%1"=="w0" set WARN_LEVEL=/W0
IF "%1"=="wx" set MSVC_OPTS=%MSVC_OPTS% /WX
IF "%1"=="pb" goto pluginbuild
SHIFT
goto argloop

:pluginbuild
set BUILD_PLUGIN=1
set MSVC_LIBS=kernel32.lib
SHIFT

set PLUGNAME=%1
SHIFT

IF NOT "%1"=="" (goto libloop) else (goto continue)

:libloop
IF "%1"=="install" (
	SET PLUGIN_INSTALL=1
	SHIFT
)
IF "%1"=="" goto continue
set MSVC_LIBS=%MSVC_LIBS% %1.lib

:libloop_shift
SHIFT
goto libloop

:continue
IF "%ARCH%"=="" goto vcerror
echo Build configuration:
echo Architecture: %ARCH%

IF "%DEBUG%"=="0" (echo Debug: disabled) else (
	set OPT_LEVEL=/Od
  set MSVC_OPTS=%MSVC_OPTS% /Z7
	set SVOUTDIR=.\out\%ARCH%dbg
  set MSVC_LINKER=%MSVC_LINKER% /DEBUG
  echo Debug: enabled
)

IF "%BUILD_PLUGIN%"=="1" (
  set COMPILER=cl /LD
	set SVPLUGDIR=%SVOUTDIR%\plugins
  set BINNAME=%PLUGNAME%
	set PROJECT_ROOT=..\cs-%PLUGNAME%
  set OBJDIR=!PROJECT_ROOT!\objs
	IF "%DEBUG%"=="1" (
		set OUTDIR=!PROJECT_ROOT!\out\%ARCH%dbg
	) else (
		set OUTDIR=!PROJECT_ROOT!\out\%ARCH%
	)
	IF NOT EXIST !PROJECT_ROOT! goto notaplugin
	IF NOT EXIST !PROJECT_ROOT!\src goto notaplugin
	echo Building plugin: %PLUGNAME%
) else set OUTDIR=%SVOUTDIR%

set BINPATH=%OUTDIR%\%BINNAME%

IF "%CLEAN%"=="0" (
	IF NOT EXIST !OBJDIR! MD !OBJDIR!
	IF NOT EXIST !OUTDIR! MD !OUTDIR!
) else goto clean

IF "%BUILD_PLUGIN%"=="1" (
  set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH% /DPLUGIN_BUILD /I.\src\
  set MSVC_LIBS=server.lib %MSVC_LIBS%
	set MSVC_LINKER=%MSVC_LINKER% /LIBPATH:%SVOUTDIR% /NOENTRY
) else (
	set "ADD_C=.\miniz\miniz.c "
	set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH%
)

set MSVC_OPTS=%MSVC_OPTS% %WARN_LEVEL% %OPT_LEVEL% /Fo%OBJDIR%\
set MSVC_OPTS=%MSVC_OPTS% /I.\miniz
set MSVC_OPTS=%MSVC_OPTS% /link %MSVC_LINKER%

IF EXIST %PROJECT_ROOT%\version.rc (
	rc /nologo /fo%OBJDIR%\version.res %PROJECT_ROOT%\version.rc
	set MSVC_OPTS=%OBJDIR%\version.res %MSVC_OPTS%
)

%COMPILER% %ADD_C%%PROJECT_ROOT%\src\*.c /I%PROJECT_ROOT%\src %MSVC_OPTS% %MSVC_LIBS%

IF "%BUILD_PLUGIN%"=="1" (
	IF "%PLUGIN_INSTALL%"=="1" (
		IF NOT EXIST !SVPLUGDIR! MD !SVPLUGDIR!
		IF "%DEBUG%"=="1" (
			copy /y !OUTDIR!\%BINNAME%.* !SVPLUGDIR!
		) else (
			copy /y !OUTDIR!\%BINNAME%.dll !SVPLUGDIR!
		)
	)
  goto end
) else (
  IF "%ERRORLEVEL%"=="0" (goto binstart) else (goto compileerror)
)

:binstart
IF "%RUNMODE%"=="0" start /D %OUTDIR% %BINNAME%
IF "%RUNMODE%"=="1" goto onerun
goto end

:onerun
pushd %OUTDIR%
%BINNAME%
popd
goto end

:cloc
set CLOCPATH=.
IF "%2" == "full" set CLOCPATH=..
cloc !CLOCPATH!
goto end

:clean
del %OBJDIR%\*.obj %OUTDIR%\*.exe %OUTDIR%\*.dll
del %OUTDIR%\*.lib %OUTDIR%\*.pdb %OUTDIR%\*.exp
goto end

:vcerror
echo Error: Script must be runned from VS Native Tools Command Prompt.
echo Note: Also you can call "vcvars64" or "vcvars32" to configure VS env.
goto end

:notaplugin
echo Looks like the specified directory is not a plugin.
goto end

:compileerror
echo Something went wrong :(
goto end

:end
endlocal
exit /B 0
