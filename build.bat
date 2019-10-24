@echo off
setlocal
set ARCH=%VSCMD_ARG_TGT_ARCH%
IF "%ARCH%"=="" goto vcerror
set CLEAN=0
set DEBUG=0
set CODE_ROOT=.
set COMPILER=cl

set SVOUTDIR=.\out\%ARCH%
set BINNAME=server.exe
set STNAME=server.lib

set ZLIB_DIR=.\zlib\%ARCH%
set ZLIB_DYNBINARY=zlib.dll
set ZLIB_STBINARY=zlib.lib

set OPENSSL_VER=1_1
set OPENSSL_DIR=.\openssl\%ARCH%
set CRYPTO_DYNBINARY=libcrypto-%OPENSSL_VER%-%ARCH%.dll
set CRYPTO_STBINARY=libcrypto.lib

set MSVC_OPTS=/MP /Gm-
set OBJDIR=objs
set MSVC_LIBS=ws2_32.lib kernel32.lib dbghelp.lib

:argloop
IF "%1"=="" goto continue
IF "%1"=="cls" cls
IF "%1"=="cloc" goto :cloc
IF "%1"=="debug" set DEBUG=1
IF "%1"=="dbg" set DEBUG=1
IF "%1"=="run" set RUNMODE=0
IF "%1"=="onerun" set RUNMODE=1
IF "%1"=="clean" set CLEAN=1
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
echo Build configuration:
echo Architecture: %ARCH%
IF "%DEBUG%"=="0" (echo Debug: disabled) else (
  set MSVC_OPTS=%MSVC_OPTS% /Z7
	set ZLIB_DYNBINARY=zlibd.dll
	set ZLIB_STBINARY=zlibd.lib
	set SVOUTDIR=.\out\%ARCH%dbg
	set OPENSSL_DIR=.\openssl\%ARCH%dbg
  set MSVC_LINKER=%MSVC_LINKER% /INCREMENTAL:NO /DEBUG /OPT:REF
  echo Debug: enabled
)

IF "%BUILD_PLUGIN%"=="1" (
  set COMPILER=cl /LD
	if "%DEBUG%"=="1" (set OUTDIR=%PLUGNAME%\out\%ARCH%dbg) else (set OUTDIR=%PLUGNAME%\out\%ARCH%)
  set BINNAME=%PLUGNAME%
  set OBJDIR=%PLUGNAME%\objs
  set CODE_ROOT=.\%PLUGNAME%
	echo Building plugin: %PLUGNAME%
) else (set OUTDIR=%SVOUTDIR%)

set SVPLUGDIR=%SVOUTDIR%\plugins
set BINPATH=%OUTDIR%\%BINNAME%

set ZLIB_STATIC=%ZLIB_DIR%\lib
set ZLIB_DYNAMIC=%ZLIB_DIR%\bin
set ZLIB_INCLUDE=%ZLIB_DIR%\include

set OPENSSL_STATIC=%OPENSSL_DIR%\lib
set OPENSSL_DYNAMIC=%OPENSSL_DIR%\bin
set OPENSSL_INCLUDE=%OPENSSL_DIR%\include

set SERVER_ZDLL=%OUTDIR%\%ZLIB_DYNBINARY%
set SERVER_ODLL=%OUTDIR%\%CRYPTO_DYNBINARY%
set MSVC_LIBS=%MSVC_LIBS% %ZLIB_STBINARY% %CRYPTO_STBINARY%

set COPYERR_LIB=zlib
set COPYERR_PATH=%ZLIB_DIR%
IF NOT EXIST %ZLIB_DYNAMIC%\%ZLIB_DYNBINARY% goto copyerr
IF NOT EXIST %ZLIB_STATIC%\%ZLIB_STBINARY% goto copyerr
IF NOT EXIST %ZLIB_INCLUDE% goto copyerr

set COPYERR_LIB=openssl
set COPYERR_PATH=%OPENSSL_DIR%
IF NOT EXIST %OPENSSL_DYNAMIC%\%CRYPTO_DYNBINARY% goto copyerr
IF NOT EXIST %OPENSSL_STATIC%\%CRYPTO_STBINARY% goto copyerr
IF NOT EXIST %OPENSSL_INCLUDE% goto copyerr

IF NOT EXIST %OBJDIR% MD %OBJDIR%
IF NOT EXIST %OUTDIR% MD %OUTDIR%
if "%CLEAN%"=="1" goto clean

IF "%BUILD_PLUGIN%"=="1" (
  set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH% /DCPLUGIN /I.\headers\
  set MSVC_LIBS=%MSVC_LIBS% %STNAME%
	set MSVC_LINKER=%MSVC_LINKER% /LIBPATH:%SVOUTDIR% /NOENTRY
) else (
  set MSVC_OPTS=%MSVC_OPTS% /Fe%BINPATH%
  copy /Y %ZLIB_DYNAMIC%\%ZLIB_DYNBINARY% %SERVER_ZDLL% 2> nul > nul
  copy /Y %OPENSSL_DYNAMIC%\%CRYPTO_DYNBINARY% %SERVER_ODLL% 2> nul > nul

)
set MSVC_OPTS=%MSVC_OPTS% /Fo%OBJDIR%\
set MSVC_OPTS=%MSVC_OPTS% /link /LIBPATH:%OPENSSL_STATIC% /LIBPATH:%ZLIB_STATIC% %MSVC_LINKER%

%COMPILER% %CODE_ROOT%\code\*.c /I%CODE_ROOT%\headers /I%OPENSSL_INCLUDE% /I%ZLIB_INCLUDE% %MSVC_OPTS% %MSVC_LIBS%

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
cloc --exclude-dir=zlib,openssl .
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
