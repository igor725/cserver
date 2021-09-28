@ECHO off
setlocal enableextensions enabledelayedexpansion
SET ARCH=%VSCMD_ARG_TGT_ARCH%
IF "%ARCH%"=="" GOTO vcerror
ECHO Changing working directory to "%~dp0"
CD /D %~dp0

SET DEBUG=0
SET SAN=0
SET PROJECT_ROOT=.
SET BUILD_PLUGIN=0
SET NOPROMPT=0
SET BINNAME=server.exe
SET GITOK=0

SET WARN_LEVEL=/W3
SET OPT_LEVEL=/O2

SET OUTDIR=out\%ARCH%
SET OBJDIR=%OUTDIR%\objs
SET SVOUTDIR=.\%OUTDIR%

SET MSVC_LINKER=/opt:ref
SET MSVC_OPTS=/FC /MP /Oi
SET MSVC_LIBS=kernel32.lib dbghelp.lib

git --version >nul
IF "%ERRORLEVEL%"=="0" (
	SET GITOK=1
	FOR /F "tokens=* USEBACKQ" %%F IN (`git describe --tags HEAD`) DO (
		SET MSVC_OPTS=%MSVC_OPTS% /DGIT_COMMIT_TAG#\"%%F\"
	)
)

:argloop
IF "%1"=="" GOTO argsdone
IF "%1"=="cls" cls
IF "%1"=="dbg" SET DEBUG=1
IF "%1"=="run" SET RUNMODE=0
IF "%1"=="noprompt" SET NOPROMPT=1
IF "%1"=="runsame" SET RUNMODE=1
IF "%1"=="od" SET OPT_LEVEL=/Od
IF "%1"=="wall" SET WARN_LEVEL=/Wall /wd4820 /wd5045
IF "%1"=="w4" SET WARN_LEVEL=/W4
IF "%1"=="w0" SET WARN_LEVEL=/W0
IF "%1"=="wx" SET MSVC_OPTS=%MSVC_OPTS% /WX
IF "%1"=="san" SET SAN=1
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
ECHO Build configuration:
ECHO Architecture: %ARCH%

IF "%DEBUG%"=="0" (
	set MSVC_OPTS=%MSVC_OPTS% /MT
	ECHO Debug: disabled
) else (
	SET SVOUTDIR=%SVOUTDIR%dbg
	SET OUTDIR=%OUTDIR%dbg
	SET OBJDIR=!OUTDIR!\objs
	SET OPT_LEVEL=/Od
	SET MSVC_OPTS=%MSVC_OPTS% /Z7 /MTd /DDEBUG
	SET MSVC_LINKER=%MSVC_LINKER% /DEBUG
	SET MSVC_OPTS=%MSVC_OPTS%
	IF "!SAN!"=="1" (
		SET MSVC_OPTS=%MSVC_OPTS% -fsanitize=address /Zi /Fd!SVOUTDIR!\server.pdb
	)
	ECHO Debug: enabled
)

IF "%BUILD_PLUGIN%"=="1" (
	SET MSVC_LINKER=%MSVC_LINKER% /DLL /NOENTRY
	SET SVPLUGDIR=%SVOUTDIR%\plugins
	SET BINNAME=%PLUGNAME%.dll
	SET PROJECT_ROOT=..\cs-%PLUGNAME%
	SET OBJDIR=!PROJECT_ROOT!\!OBJDIR!
	SET OUTDIR=!PROJECT_ROOT!\!OUTDIR!
	IF NOT EXIST !PROJECT_ROOT! GOTO notaplugin
	IF NOT EXIST !PROJECT_ROOT!\src GOTO notaplugin
	ECHO Building plugin: %PLUGNAME%
) else (
	SET MSVC_LINKER=%MSVC_LINKER% /subsystem:console
	SET MSVC_LIBS=!MSVC_LIBS! ws2_32.lib
	IF NOT EXIST !OUTDIR! MD !OUTDIR!
	IF NOT EXIST !OBJDIR! MD !OBJDIR!
	GOTO detectzlib
)

:zlibok
SET BINPATH=%OUTDIR%\%BINNAME%
IF "%BUILD_PLUGIN%"=="1" (
  SET MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH% /DCORE_BUILD_PLUGIN /I.\src\
  SET MSVC_LIBS=server.lib %MSVC_LIBS%
	SET MSVC_LINKER=%MSVC_LINKER% /libpath:%SVOUTDIR%
) else (
	SET MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH%
)

SET MSVC_OPTS=%MSVC_OPTS% %WARN_LEVEL% %OPT_LEVEL% /Fo%OBJDIR%\
SET MSVC_OPTS=%MSVC_OPTS% /link %MSVC_LINKER%
SET SOURCES=%PROJECT_ROOT%\src\*.c

IF EXIST %PROJECT_ROOT%\version.rc (
	RC /nologo /Fo%OBJDIR%\version.res %PROJECT_ROOT%\version.rc
	SET SOURCES=%SOURCES% %OBJDIR%\version.res
)

CL %SOURCES% /I%PROJECT_ROOT%\src %MSVC_OPTS% %MSVC_LIBS%
IF NOT "%ERRORLEVEL%"=="0" GOTO compileerror

IF "%BUILD_PLUGIN%"=="1" (
	IF "%PLUGIN_INSTALL%"=="1" (
		IF NOT EXIST !SVPLUGDIR! MD !SVPLUGDIR!
		COPY /Y !OUTDIR!\%PLUGNAME%.dll !SVPLUGDIR!
		IF "%DEBUG%"=="1" (
			COPY /Y !OUTDIR!\%PLUGNAME%.pdb !SVPLUGDIR!
		)
	)
  GOTO endok
) else GOTO binstart

:detectzlib
IF NOT EXIST ".\zlib\" GOTO nozlib
IF NOT EXIST ".\zlib\zlib.h" GOTO nozlib
IF NOT EXIST ".\zlib\zconf.h" GOTO nozlib
set MSVC_OPTS=%MSVC_OPTS% /I.\zlib\
FOR /F "tokens=* USEBACKQ" %%F IN (`WHERE /R .\zlib\ z*.dll`) DO (
	SET ZLIB_DYNAMIC=%%F
)
IF "%ZLIB_DYNAMIC%"=="" (
	echo ZLIB empty
	GOTO makezlib_st1
) ELSE (
	IF NOT EXIST "%ZLIB_DYNAMIC%" (
		GOTO makezlib_st1
	) ELSE (
		GOTO copyzlib
	)
)

:makezlib_st1
IF NOT EXIST ".\zlib\win32\Makefile.msc" (
	GOTO nozlib
) ELSE (
	GOTO makezlib_st2
)

:nozlib
IF "%NOPROMPT%"=="0" (
	ECHO zlib not found in root directory.
	ECHO Would you like the script to automatically clone and build zlib library?
	ECHO Note: The zlib repo will be cloned from Mark Adler's GitHub, then compiled.
	ECHO Warning: If zlib directory exists it will be removed!
	SET /P ZQUESTION="[Y/n]>"
	IF "!ZQUESTION!"=="n" GOTO end
	IF "!ZQUESTION!"=="N" GOTO end
	IF "!ZQUESTION!"=="" GOTO downzlib
	IF "!ZQUESTION!"=="y" GOTO downzlib
	IF "!ZQUESTION!"=="Y" GOTO downzlib
	GOTO end	
)

:downzlib
IF "%GITOK%"=="0" (
	ECHO Looks like you don't have Git for Windows
	ECHO You can download it from https://git-scm.com/download/win
) ELSE (
	RMDIR /S /Q .\zlib
	git clone https://github.com/madler/zlib
	GOTO makezlib_st2
)

:makezlib_st2
PUSHD ".\zlib\"
nmake /f win32\Makefile.msc
POPD
GOTO detectzlib

:copyzlib
COPY "%ZLIB_DYNAMIC%" "%SVOUTDIR%"
IF "%DEBUG%"=="1" (
	COPY "!ZLIB_DYNAMIC:~0,-3!pdb" "%SVOUTDIR%"
)
goto zlibok

:binstart
IF "%RUNMODE%"=="0" start /D %OUTDIR% %BINNAME%
IF "%RUNMODE%"=="1" GOTO onerun
GOTO endok

:onerun
PUSHD %OUTDIR%
%BINNAME%
POPD
GOTO endok

:vcerror
ECHO Error: Script must be runned from VS Native Tools Command Prompt.
ECHO Note: Also you can call "vcvars64" or "vcvars32" to configure VS env.
GOTO end

:notaplugin
ECHO Looks like the specified directory is not a plugin.
GOTO end

:compileerror
ECHO Something went wrong :(
GOTO end

:end
endlocal
EXIT /B 2

:endok
endlocal
EXIT /B 0
