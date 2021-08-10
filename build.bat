@ECHO off
setlocal enableextensions enabledelayedexpansion
ECHO Changing working directory to "%~dp0"
CD /D %~dp0

SET ARCH=%VSCMD_ARG_TGT_ARCH%
SET DEBUG=0
SET PROJECT_ROOT=.
SET BUILD_PLUGIN=0
SET ECODE=0

SET BINNAME=server.exe

SET WARN_LEVEL=/W3
SET OPT_LEVEL=/O2

SET OUTDIR=out\%ARCH%
SET OBJDIR=objs\%ARCH%
SET SVOUTDIR=.\%OUTDIR%

SET MSVC_LINKER=/opt:ref
SET MSVC_OPTS=/MP /Oi /DZLIB_DLL /DZLIB_WINAPI
SET MSVC_LIBS=kernel32.lib

git --version >nul
IF %ERRORLEVEL% EQ 0 (
	FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --short HEAD`) DO (
		SET MSVC_OPTS=%MSVC_OPTS% /DGIT_COMMIT_SHA#\"%%F\"
	)
)

:argloop
IF "%1"=="" GOTO argsdone
IF "%1"=="cls" cls
IF "%1"=="dbg" SET DEBUG=1
IF "%1"=="run" SET RUNMODE=0
IF "%1"=="runsame" SET RUNMODE=1
IF "%1"=="od" SET OPT_LEVEL=/Od
IF "%1"=="wall" SET WARN_LEVEL=/Wall /wd4820 /wd5045 /wd4710
IF "%1"=="w4" SET WARN_LEVEL=/W4
IF "%1"=="w0" SET WARN_LEVEL=/W0
IF "%1"=="wx" SET MSVC_OPTS=%MSVC_OPTS% /WX
IF "%1"=="pb" SET BUILD_PLUGIN=1
IF "%1"=="pbu" SET BUILD_PLUGIN=2
SHIFT
IF NOT "%BUILD_PLUGIN%"=="0" GOTO pluginbuild
GOTO argloop

:pluginbuild
SET PLUGNAME=%1
SHIFT

IF "%BUILD_PLUGIN%"=="2" (
	PUSHD ..\cs-%PLUGNAME%
	git pull
	POPD
	SET BUILD_PLUGIN=1
)

IF "%1"=="" GOTO argsdone

:libloop
IF "%1"=="install" (
	SET PLUGIN_INSTALL=1
	SHIFT
)
IF "%1"=="" GOTO argsdone
SET MSVC_LIBS=%MSVC_LIBS% %1.lib

:libloop_shift
SHIFT
GOTO libloop

:argsdone
IF "%ARCH%"=="" GOTO vcerror
ECHO Build configuration:
ECHO Architecture: %ARCH%

IF "%DEBUG%"=="0" (
	set MSVC_OPTS=%MSVC_OPTS% /DRELEASE_BUILD
	ECHO Debug: disabled
) else (
	SET OPT_LEVEL=/Od
  SET MSVC_OPTS=%MSVC_OPTS% /Z7
	SET MSVC_LIBS=%MSVC_LIBS% dbghelp.lib
	SET SVOUTDIR=%SVOUTDIR%dbg
	SET OUTDIR=%OUTDIR%dbg
  ECHO Debug: enabled
)

IF "%BUILD_PLUGIN%"=="1" (
  SET MSVC_LINKER=%MSVC_LINKER% /DLL /NOENTRY
	SET SVPLUGDIR=%SVOUTDIR%\plugins
  SET BINNAME=%PLUGNAME%.dll
	SET PROJECT_ROOT=..\cs-%PLUGNAME%
  SET OBJDIR=!PROJECT_ROOT!\%OBJDIR%
	SET OUTDIR=!PROJECT_ROOT!\%OUTDIR%
	IF NOT EXIST !PROJECT_ROOT! GOTO notaplugin
	IF NOT EXIST !PROJECT_ROOT!\src GOTO notaplugin
	ECHO Building plugin: %PLUGNAME%
) else (
	SET MSVC_LINKER=%MSVC_LINKER% /subsystem:console
	IF NOT EXIST "%SVOUTDIR%" MKDIR %SVOUTDIR%
	SET ZLIB_LINK=z.lib
	FOR /F "tokens=* USEBACKQ" %%F IN (`DIR /B .\zlib\lib%ARCH%\*.lib`) DO (
		SET ZLIB_LINK=%%F
	)
	SET ZLIB_DYNAMIC=!ZLIB_LINK:~0,-3!dll
	SET ZLIB_DEBUG=!ZLIB_LINK:~0,-3!pdb
	SET MSVC_LIBS=%MSVC_LIBS% ws2_32.lib wininet.lib advapi32.lib !ZLIB_LINK!
	IF NOT EXIST "%SVOUTDIR%\!ZLIB_DYNAMIC!" (
		COPY ".\zlib\lib%ARCH%\!ZLIB_DYNAMIC!" "%SVOUTDIR%"
	)
	IF "%DEBUG%"=="1" IF NOT EXIST "%SVOUTDIR%\!ZLIB_DEBUG!" (
		COPY ".\zlib\lib%ARCH%\!ZLIB_DEBUG!" "%SVOUTDIR%"
	)
)

IF NOT EXIST !OBJDIR! MD !OBJDIR!
IF NOT EXIST !OUTDIR! MD !OUTDIR!

SET BINPATH=%OUTDIR%\%BINNAME%
IF "%BUILD_PLUGIN%"=="1" (
  SET MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH% /DPLUGIN_BUILD /I.\src\
  SET MSVC_LIBS=server.lib %MSVC_LIBS%
	SET MSVC_LINKER=%MSVC_LINKER% /libpath:%SVOUTDIR%
) else (
	SET MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH%
)

SET MSVC_OPTS=%MSVC_OPTS% %WARN_LEVEL% %OPT_LEVEL% /Fo%OBJDIR%\
set MSVC_OPTS=%MSVC_OPTS% /I.\zlib\include\
SET MSVC_OPTS=%MSVC_OPTS% /link /libpath:.\zlib\lib%ARCH%\ %MSVC_LINKER%

IF EXIST %PROJECT_ROOT%\version.rc (
	RC /nologo /fo%OBJDIR%\version.res %PROJECT_ROOT%\version.rc
	SET MSVC_OPTS=%OBJDIR%\version.res %MSVC_OPTS%
)

SET SRC_LIST=
FOR /F "tokens=* USEBACKQ" %%A IN (`dir /b /a-d %PROJECT_ROOT%\src\*.c`) DO (
	SET SRC_LIST=!SRC_LIST! %PROJECT_ROOT%\src\%%A
)
CL%SRC_LIST% /I%PROJECT_ROOT%\src %MSVC_OPTS% %MSVC_LIBS%
IF NOT "%ERRORLEVEL%"=="0" GOTO compileerror

IF "%BUILD_PLUGIN%"=="1" (
	IF "%PLUGIN_INSTALL%"=="1" (
		IF NOT EXIST !SVPLUGDIR! MD !SVPLUGDIR!
		COPY /Y !OUTDIR!\%PLUGNAME%.dll !SVPLUGDIR!
		IF "%DEBUG%"=="1" (
			COPY /Y !OUTDIR!\%PLUGNAME%.pdb !SVPLUGDIR!
		)
	)
  GOTO end
) else GOTO binstart

:binstart
IF "%RUNMODE%"=="0" start /D %OUTDIR% %BINNAME%
IF "%RUNMODE%"=="1" GOTO onerun
GOTO end

:onerun
PUSHD %OUTDIR%
%BINNAME%
POPD
GOTO end

:vcerror
ECHO Error: Script must be runned from VS Native Tools Command Prompt.
ECHO Note: Also you can call "vcvars64" or "vcvars32" to configure VS env.
GOTO end

:notaplugin
ECHO Looks like the specified directory is not a plugin.
GOTO end

:compileerror
ECHO Something went wrong :(
SET ECODE=1
GOTO end

:end
endlocal
EXIT /B %ECODE%
