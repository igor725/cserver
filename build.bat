@ECHO off
setlocal enableextensions enabledelayedexpansion
CALL misc\vsdetect.bat
IF !ERRORLEVEL! NEQ 0 GOTO end
CD /D %~dp0

SET RUNSERVER=0
SET DEBUG=0
SET SAN=0
SET ROOT=.
SET PLUGIN_BUILD=0
SET PLUGIN_ARGS=
SET NOPROMPT=0
SET OUTBIN=server.exe
SET GITOK=0
SET GITUPD=0

SET WARN=/W3
SET OPT=/O2

SET OUTDIR=!ROOT!\out\!ARCH!
SET OBJDIR=!ROOT!\out\.objs\!ARCH!
SET SERVER_OUTROOT=!OUTDIR!

SET LDFLAGS=/link
SET CFLAGS=/FC /MP /Oi
SET LIBS=dbghelp.lib

:argloop
IF "%1"=="" GOTO argsdone
IF "%1"=="cls" cls
IF "%1"=="dbg" SET DEBUG=1
IF "%1"=="run" SET RUNSERVER=1
IF "%1"=="noprompt" SET NOPROMPT=1
IF "%1"=="od" SET OPT=/Od
IF "%1"=="wall" SET WARN=/Wall /wd4820 /wd5045
IF "%1"=="w4" SET WARN=/W4
IF "%1"=="w0" SET WARN=/W0
IF "%1"=="wx" SET CFLAGS=%CFLAGS% /WX
IF "%1"=="san" SET SAN=1
IF "%1"=="pb" SET PLUGIN_BUILD=1
IF "%1"=="upd" IF !GITOK! EQU 1 SET GITUPD=1
SHIFT
IF !PLUGIN_BUILD! EQU 1 GOTO pluginbuild
GOTO argloop

:pluginbuild
SET PLUGNAME=%1
SHIFT

SET LDFLAGS=!LDFLAGS! /DLL
SET OUTBIN=!PLUGNAME!.dll
SET ROOT=..\cs-!PLUGNAME!
SET OBJDIR=!ROOT!\!OBJDIR!
SET OUTDIR=!ROOT!\!OUTDIR!
IF NOT EXIST !ROOT! GOTO notaplugin
IF NOT EXIST !ROOT!\src GOTO notaplugin
ECHO Building plugin: !PLUGNAME!

:subargloop
IF "%1"=="" GOTO argsdone

IF "%1"=="install" (
	SET PLUGIN_INSTALL=1
) ELSE (
	SET PLUGIN_ARGS=!PLUGIN_ARGS!%1 
)
SHIFT
GOTO subargloop

:argsdone
CALL misc\gitthing.bat
IF !ERRORLEVEL! NEQ 0 GOTO end
ECHO Build configuration:
ECHO Architecture: !ARCH!

IF !DEBUG! EQU 0 (
	SET CFLAGS=!CFLAGS! /MT
	SET LDFLAGS=!LDFLAGS! /RELEASE /OPT:REF
	ECHO Debug: disabled
) else (
	SET SERVER_OUTROOT=!SERVER_OUTROOT!dbg
	SET OUTDIR=!OUTDIR!dbg
	SET OBJDIR=!OBJDIR!dbg
	SET OPT=/Od
	SET CFLAGS=!CFLAGS! /Z7 /MTd /DDEBUG /DCORE_BUILD_DEBUG
	SET LDFLAGS=!LDFLAGS! /DEBUG
	IF !SAN! EQU 1 (
		SET CFLAGS=!CFLAGS! -fsanitize=address /Zi /Fd!SERVER_OUTROOT!\server.pdb
	)
	ECHO Debug: enabled
)

IF NOT EXIST !OBJDIR! MD !OBJDIR!
IF NOT EXIST !OUTDIR! MD !OUTDIR!

IF !PLUGIN_BUILD! EQU 0 (
	SET LDFLAGS=!LDFLAGS! /SUBSYSTEM:CONSOLE
	SET LIBS=!LIBS! ws2_32.lib
	CALL misc\zdetect.bat
	IF !ERRORLEVEL! EQU 0 (
		GOTO compile
	) ELSE (
		GOTO compileerror
	)
) else (
	IF NOT EXIST !SERVER_OUTROOT! GOTO noserver
	IF NOT EXIST !SERVER_OUTROOT!\server.lib GOTO noserver
)

:compile
IF EXIST !ROOT!\vars.bat (
	CALL !ROOT!\vars.bat
	IF !ERRORLEVEL! NEQ 0 GOTO compileerror
)

SET BINPATH=!OUTDIR!\!OUTBIN!
IF !PLUGIN_BUILD! EQU 1 (
	SET CFLAGS=!CFLAGS! /Fe!BINPATH! /DCORE_BUILD_PLUGIN /I.\src\
	SET LIBS=server.lib !LIBS!
	SET LDFLAGS=!LDFLAGS! /LIBPATH:!SERVER_OUTROOT!
) else (
	SET CFLAGS=!CFLAGS! /Fe!BINPATH!
)

SET CFLAGS=!CFLAGS! !WARN! !OPT! /Fo!OBJDIR!\
SET SOURCES=!ROOT!\src\*.c

IF EXIST !ROOT!\version.rc (
	SET RCCFLAGS=/NOLOGO /Fo!OBJDIR!\version.res
	IF !DEBUG! EQU 1 (
		SET RCCFLAGS=!RCCFLAGS! /DDEBUG
	)
	RC !RCCFLAGS! !ROOT!\version.rc
	SET SOURCES=!SOURCES! !OBJDIR!\version.res
)

CL !SOURCES! /I!ROOT!\src !CFLAGS! !LDFLAGS! !LIBS!
IF !ERRORLEVEL! NEQ 0 GOTO compileerror

IF !PLUGIN_BUILD! EQU 1 (
	SET SVPLUGDIR=!SERVER_OUTROOT!\plugins
	IF !PLUGIN_INSTALL! EQU 1 (
		IF NOT EXIST !SVPLUGDIR! MD !SVPLUGDIR!
		COPY /Y !OUTDIR!\!PLUGNAME!.dll !SVPLUGDIR!
		IF !DEBUG! EQU 1 COPY /Y !OUTDIR!\!PLUGNAME!.pdb !SVPLUGDIR!
	)
  GOTO endok
) else GOTO binstart

:binstart
IF !RUNSERVER! EQU 0 GOTO endok
PUSHD !OUTDIR!
!OUTBIN!
POPD
GOTO endok

:notaplugin
ECHO Looks like the specified directory is not a plugin.
GOTO end

:noserver
ECHO Before compiling plugins, you must compile the server.
GOTO end

:compileerror
ECHO Something went wrong :(
GOTO end

:end
endlocal
EXIT /B 1

:endok
endlocal
EXIT /B 0
