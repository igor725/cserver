@echo off
setlocal
set ARCH=%VSCMD_ARG_TGT_ARCH%
set CLEAN=0
set DEBUG=0
set PROJECT_ROOT=.
set COMPILER=cl

set SVOUTDIR=.\out\%ARCH%
set BINNAME=server.exe
set STNAME=server.lib

set ZLIB_DIR=.\zlib\%ARCH%
set ZLIB_DYNBINARY=zlib.dll
set ZLIB_STBINARY=zlib.lib

set WARN_LEVEL=/W3
set OPT_LEVEL=/O2

set MSVC_LINKER=/INCREMENTAL:NO /OPT:REF /SUBSYSTEM:CONSOLE
set MSVC_OPTS=/MP /GL /Oi /Gy /fp:fast
set OBJDIR=objs
set MSVC_LIBS=ws2_32.lib kernel32.lib dbghelp.lib advapi32.lib
FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --short HEAD`) DO (
	set MSVC_OPTS=%MSVC_OPTS% /DGIT_COMMIT_SHA#"\"%%F\""
)

:argloop
IF "%1"=="" goto continue
IF "%1"=="cls" cls
IF "%1"=="cloc" goto :cloc
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
SHIFT

set PLUGNAME=%1
SHIFT

IF "%1"=="install" (
	set PLUGINSTALL=1
	SHIFT
)
IF NOT "%1"=="" (goto libloop) else (goto continue)

:libloop
IF "%1"=="" goto continue
set MSVC_LIBS=%MSVC_LIBS% %1.lib
SHIFT
goto libloop

:continue
IF "%ARCH%"=="" goto vcerror
echo Build configuration:
echo Architecture: %ARCH%
echo Commit: %COMMIT_SHA%

IF "%DEBUG%"=="0" (echo Debug: disabled) else (
	set OPT_LEVEL=/Od
  set MSVC_OPTS=%MSVC_OPTS% /Z7
	set ZLIB_DYNBINARY=zlibd.dll
	set ZLIB_STBINARY=zlibd.lib
	set SVOUTDIR=.\out\%ARCH%dbg
  set MSVC_LINKER=%MSVC_LINKER% /DEBUG
  echo Debug: enabled
)

IF "%BUILD_PLUGIN%"=="1" (
  set COMPILER=cl /LD
	if "%DEBUG%"=="1" (set OUTDIR=%PLUGNAME%\out\%ARCH%dbg) else (set OUTDIR=%PLUGNAME%\out\%ARCH%)
  set BINNAME=%PLUGNAME%
  set OBJDIR=%PLUGNAME%\objs
  set PROJECT_ROOT=.\%PLUGNAME%
	echo Building plugin: %PLUGNAME%
) else (set OUTDIR=%SVOUTDIR%)

set SVPLUGDIR=%SVOUTDIR%\plugins
set BINPATH=%OUTDIR%\%BINNAME%

set ZLIB_STATIC=%ZLIB_DIR%\lib
set ZLIB_DYNAMIC=%ZLIB_DIR%\bin
set ZLIB_INCLUDE=%ZLIB_DIR%\include

set SERVER_ZDLL=%OUTDIR%\%ZLIB_DYNBINARY%
set MSVC_LIBS=%MSVC_LIBS% %ZLIB_STBINARY%

IF "%CLEAN%"=="0" (
	set COPYERR_LIB=zlib
	set COPYERR_PATH=%ZLIB_DIR%

	IF NOT EXIST %ZLIB_DYNAMIC%\%ZLIB_DYNBINARY% goto copyerr
	IF NOT EXIST %ZLIB_STATIC%\%ZLIB_STBINARY% goto copyerr
	IF NOT EXIST %ZLIB_INCLUDE% goto copyerr

	IF NOT EXIST %OBJDIR% MD %OBJDIR%
	IF NOT EXIST %OUTDIR% MD %OUTDIR%
) else ( goto clean )

IF "%BUILD_PLUGIN%"=="1" (
  set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH% /DPLUGIN_BUILD /I.\headers\
  set MSVC_LIBS=%MSVC_LIBS% %STNAME%
	set MSVC_LINKER=%MSVC_LINKER% /LIBPATH:%SVOUTDIR% /NOENTRY
) else (
  copy /Y %ZLIB_DYNAMIC%\%ZLIB_DYNBINARY% %SERVER_ZDLL% 2> nul > nul
	set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH%
)

set MSVC_OPTS=%MSVC_OPTS% %WARN_LEVEL% %OPT_LEVEL% /Fo%OBJDIR%\
set MSVC_OPTS=%MSVC_OPTS% /link /LIBPATH:%ZLIB_STATIC% %MSVC_LINKER%

IF EXIST %PROJECT_ROOT%\version.rc (
	rc /nologo /fo%OBJDIR%\version.res %PROJECT_ROOT%\version.rc
	set MSVC_OPTS=%OBJDIR%\version.res %MSVC_OPTS%
)

%COMPILER% %PROJECT_ROOT%\code\*.c /I%PROJECT_ROOT%\headers /I%ZLIB_INCLUDE% %MSVC_OPTS% %MSVC_LIBS%

IF "%BUILD_PLUGIN%"=="1" (
	IF "%PLUGINSTALL%"=="1" (
		IF NOT EXIST %SVPLUGDIR% MD %SVPLUGDIR%
		IF "%DEBUG%"=="1" (
			copy /y %OUTDIR%\%BINNAME%.* %SVPLUGDIR%
		) else (
			copy /y %OUTDIR%\%BINNAME%.dll %SVPLUGDIR%
		)
	)
  goto :end
) else (
  IF "%ERRORLEVEL%"=="0" (goto binstart) else (goto compileerror)
)

:binstart
IF "%RUNMODE%"=="0" start /D %OUTDIR% %BINNAME%
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

:clean
del %OBJDIR%\*.obj %OUTDIR%\*.exe %OUTDIR%\*.dll
del %OUTDIR%\*.lib %OUTDIR%\*.pdb %OUTDIR%\*.exp
goto end

:vcerror
echo Error: Script must be runned from VS Native Tools Command Prompt.
echo Note: Also you can call "vcvars64" or "vcvars32" to configure VS env.
goto end

:copyerr
echo Please download %COPYERR_LIB% binaries for Microsoft Visual Studio %VisualStudioVersion% from this site: https://www.libs4win.com/lib%COPYERR_LIB%/.
echo After downloading, unzip the archive to the "%COPYERR_PATH%" folder in the project root.
goto end

:compileerror
echo Something went wrong :(
goto end

:end
endlocal
exit /B 0
