@REM Сделать тут умное обновление
IF "!GITOK!"=="1" (
	IF "!GITUPD!"=="1" (
		git -C !ROOT! fetch && git -C !ROOT! merge --ff-only
		IF NOT "!ERRORLEVEL!"=="0" GOTO gitfail
	)

	SET TAG_INSTALLED=0

	FOR /F "tokens=* USEBACKQ" %%F IN (`git -C "!ROOT!" describe --tags HEAD 2^> nul`) DO (
		SET CFLAGS=!CFLAGS! /DGIT_COMMIT_TAG#\"%%F\"
		SET TAG_INSTALLED=1
	)
	
	IF "!TAG_INSTALLED!"=="0" (
		FOR /F "tokens=* USEBACKQ" %%F IN (`git -C "!ROOT!" rev-parse --short HEAD`) DO 2> nul (
			SET CFLAGS=!CFLAGS! /DGIT_COMMIT_TAG#\"%%F\"
		)	
	)

	EXIT /B 0
)

:gitfail
ECHO GITTHING: Failed to fetch repository changes
EXIT /B 1
