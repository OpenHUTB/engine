@echo off

set "Root=%~dp0"
set "RunUAT=%Root%Engine\Build\BatchFiles\RunUAT.bat"
set "BuildGraphScript=%Root%Engine\Build\InstalledEngineBuild.xml"

if not exist "%RunUAT%" (
	echo Could not find "%RunUAT%".
	pause
	exit /b 1
)

if not exist "%BuildGraphScript%" (
	echo Could not find "%BuildGraphScript%".
	pause
	exit /b 1
)

if exist "%ProgramW6432%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
	echo Using Visual Studio 2022 for packaging.
    call "%RunUAT%" BuildGraph -script="%BuildGraphScript%" -target="Make Installed Build Win64" -nosign -set:GameConfigurations=Development;Shipping -set:WithWin64=true -set:WithWin32=false -set:WithMac=false -set:WithAndroid=false -set:WithIOS=false -set:WithTVOS=false -set:WithLinux=false -set:WithLinuxAArch64=false -set:WithDDC=false -set:VS2022=true -clean
) else (
	echo Using Visual Studio 2019 for packaging.
    call "%RunUAT%" BuildGraph -script="%BuildGraphScript%" -target="Make Installed Build Win64" -nosign -set:GameConfigurations=Development;Shipping -set:WithWin64=true -set:WithWin32=false -set:WithMac=false -set:WithAndroid=false -set:WithIOS=false -set:WithTVOS=false -set:WithLinux=false -set:WithLinuxAArch64=false -set:WithDDC=false -clean
)


pause
exit /b %ERRORLEVEL%