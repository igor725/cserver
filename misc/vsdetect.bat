SET ARCH=%VSCMD_ARG_TGT_ARCH%
IF "%ARCH%"=="" GOTO vcerror

WHERE /Q CL.EXE
IF "%ERRORLEVEL%"=="0" (
	EXIT /B 0
)

:vcerror
ECHO Error: You must open a Visual Studio Command Prompt to run this script.
ECHO Note: If you don't have Visual Studio C++ installed, you can download it here:
ECHO https://visualstudio.microsoft.com/downloads/
EXIT /B 1
