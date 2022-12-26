@REM Сделать тут умное обновление
git --version >nul
IF !ERRORLEVEL! EQU 0 SET GITOK=1

IF !GITOK! EQU 1 (
	IF !GITUPD! EQU 1 (
		git -C !ROOT! fetch && git -C !ROOT! merge --ff-only
		IF NOT "!ERRORLEVEL!"=="0" GOTO :gitfail
	)

	SET TAG_INSTALLED=0

	FOR /F "tokens=* USEBACKQ" %%F IN (`git -C "!ROOT!" describe --tags HEAD 2^> nul`) DO (
		SET CFLAGS=!CFLAGS! /DGIT_COMMIT_TAG#\"%%F\"
		SET TAG_INSTALLED=1
	)

	IF !TAG_INSTALLED! EQU 0 (
		FOR /F "tokens=* USEBACKQ" %%F IN (`git -C "!ROOT!" rev-parse --short HEAD 2^> nul`) DO 2> nul (
			SET CFLAGS=!CFLAGS! /DGIT_COMMIT_TAG#\"%%F\"
		)
	)

	EXIT /B 0
)

ECHO GITTHING: GIT binary was not found
EXIT /B 0

:gitfail
ECHO GITTHING: Failed to fetch repository changes
EXIT /B 1
