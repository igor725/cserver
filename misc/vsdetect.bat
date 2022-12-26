WHERE /Q CL.EXE
IF !ERRORLEVEL! NEQ 0 (
	IF "!PROCESSOR_ARCHITECTURE!"=="x86" (
		SET VCFILE=vcvars32
		CALL :testforvc "!ProgramFiles!\Microsoft Visual Studio"
		IF !ERRORLEVEL! NEQ 0 GOTO :vcerror
	) ELSE IF "!PROCESSOR_ARCHITECTURE!"=="AMD64" (
		SET VCFILE=vcvars64
		CALL :testforvc "!ProgramFiles(x86)!\Microsoft Visual Studio"
		IF !ERRORLEVEL! NEQ 0 CALL :testforvc "!ProgramFiles!\Microsoft Visual Studio"
		IF !ERRORLEVEL! NEQ 0 GOTO :vcerror
	) ELSE (
		ECHO Unknown architecture
		EXIT /B 1
	)
)

:vcok
SET ARCH=!VSCMD_ARG_TGT_ARCH!
IF "!ARCH!"=="" GOTO vcerror
EXIT /B 0

:testforvc
FOR /F "tokens=* USEBACKQ" %%F IN (`DIR /B /AD %* 2^> nul`) DO (
	SET VCVARSALL=%1\%%F\BuildTools\VC\Auxiliary\Build\!VCFILE!.bat
	IF EXIST !VCVARSALL! (
		CALL !VCVARSALL!
		IF !ERRORLEVEL! NEQ 0 GOTO :vcerror
		GOTO :vcok
	)
)
EXIT /B 0

:vcerror
ECHO You don't have Visual Studio C++ BuildTools installed, you can download it here:
ECHO https://visualstudio.microsoft.com/downloads/
EXIT /B 1
