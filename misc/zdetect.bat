:detectzlib
SET ZFOLDER=..\zlib
IF EXIST ".\zlib.path" (
	FOR /F "delims=" %%F IN (.\zlib.path) DO (
		IF NOT "%%F"=="" SET ZFOLDER=%%F
	)
) else (
	ECHO Type absolute or relative path ^(without quotes^) to the folder that containing zlib repo.
	ECHO If specified folder is empty or does not contain zlib source code, it will be cloned and
	ECHO builded by this script automatically.
	ECHO Hint: If you leave the path field empty, the script will use "!ZFOLDER!" by default.
	:zfolderloop
	SET /P UZF="zlib path> "
	IF NOT "!UZF!"=="" (
		IF NOT EXIST "!UZF!" MD "!UZF!"
		SET ZFOLDER=!UZF!
	)
	ECHO !ZFOLDER!>.\zlib.path
)

IF NOT EXIST "!ZFOLDER!\" GOTO nozlib
IF NOT EXIST "!ZFOLDER!\zlib.h" GOTO nozlib
IF NOT EXIST "!ZFOLDER!\zconf.h" GOTO nozlib
SET INCLUDE=%INCLUDE%;!ZFOLDER!\
FOR /F "tokens=* USEBACKQ" %%F IN (`WHERE /R "!ZFOLDER!\win32\!ARCH!" z*.dll`) DO (
	@REM Добавить проверку найденной DLLки на совместимость с текущей системой
	SET ZLIB_DYNAMIC=%%F
)
IF "%ZLIB_DYNAMIC%"=="" (
	GOTO makezlib_st1
) ELSE (
	IF NOT EXIST "!ZLIB_DYNAMIC!" (
		GOTO makezlib_st1
	) ELSE (
		GOTO copyzlib
	)
)

:makezlib_st1
IF NOT EXIST "!ZFOLDER!\win32\Makefile.msc" (
	GOTO nozlib
) ELSE (
	GOTO makezlib_st2
)

:nozlib
IF "%NOPROMPT%"=="0" (
	ECHO zlib not found in "!ZFOLDER!".
	ECHO Would you like the script to automatically clone and build zlib library?
	ECHO Note: The zlib repo will be cloned from Mark Adler's GitHub, then compiled.
	ECHO Warning: If "!ZFOLDER!" exists it will be removed!
	SET /P ZQUESTION="[Y/n]>"
	IF "!ZQUESTION!"=="" GOTO downzlib
	IF "!ZQUESTION!"=="y" GOTO downzlib
	IF "!ZQUESTION!"=="Y" GOTO downzlib
	GOTO zclonefail
)

:downzlib
IF "%GITOK%"=="0" (
	ECHO Looks like you don't have Git for Windows
	ECHO You can download it from https://git-scm.com/download/win
) ELSE (
	RMDIR /S /Q "!ZFOLDER!"
	git clone https://github.com/madler/zlib "!ZFOLDER!"
	GOTO makezlib_st2
)

:makezlib_st2
IF NOT EXIST "!ZFOLDER!\win32\!ARCH!" MD "!ZFOLDER!\win32\!ARCH!"
PUSHD "!ZFOLDER!\win32\!ARCH!"
NMAKE /F ..\Makefile.msc TOP=..\..\
IF "!ERRORLEVEL!"=="0" (
	POPD
	GOTO detectzlib
) else (
	POPD
	GOTO zcompilefail
)

:copyzlib
FOR /F "tokens=* USEBACKQ" %%F IN (`WHERE /R !SERVER_OUTROOT! zlib1.dll`) DO (
	IF EXIST "%%F" GOTO zlibok
)
COPY "%ZLIB_DYNAMIC%" "%SERVER_OUTROOT%\zlib1.dll"
IF "%DEBUG%"=="1" (
	COPY "!ZLIB_DYNAMIC:~0,-3!pdb" "!SERVER_OUTROOT!\zlib1.pdb"
)
GOTO zlibok

:zclonefail
ECHO Error: Failed to clone zlib repo
EXIT /B 1

:zcompilefail
ECHO Error: failed to compile zlib
EXIT /B 1

:zlibok
EXIT /B 0
