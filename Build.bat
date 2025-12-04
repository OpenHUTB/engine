@echo off
setlocal
chcp 65001


:: call setEnv64.bat
:: %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

if exist "%programfiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
	call "%programfiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
	call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)


:: 检查是否存在 ..\..\..\dependencies\ue4-gitdeps 目录，存在则拷贝到 .git\ue4-gitdeps
for %%I in ("%~dp0..\..\..") do set "out_dir=%%~fI"
set dep_dir="%out_dir%dependencies\ue4-gitdeps"
echo dep_dir=%dep_dir%
if exist "%dep_dir%" (
	echo existing dependencies\ue4-gitdeps found.
	if not exist "%~dp0\.git\ue4-gitdeps" (
		echo Copying ue4-gitdeps from dependencies to .git directory...
		xcopy /E /I /Y "%~dp0..\..\..\dependencies\ue4-gitdeps" "%~dp0\.git\ue4-gitdeps"
		echo Copy dependencies complete.
	)
)
:: pause
:: exit /b 0

:: 使用 --force 选项来跳过: Checking dependencies... overwrite your changes (y/n)
call Setup.bat --force

call GenerateProjectFiles.bat


:: %ProgramFiles(x86)%=C:\Program Files (x86)
:: %ProgramW6432%=C:\Program Files

:: 没有构建 UE4Editor.exe
:: call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" Engine\Intermediate\ProjectFiles\UE4.vcxproj

:: 注意：双引号必须且只能将包含空格的目录
if exist "%programfiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
	echo "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\bin\MSBuild.exe" UE4.sln  /p:Configuration="Development Editor" /p:Platform="Win64" /p:Project="UnrealBuildTool" /p:OutputPath=.\
	call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\bin\MSBuild.exe" UE4.sln  /p:Configuration="Development Editor" /p:Platform="Win64" /p:Project="UnrealBuildTool" /p:OutputPath=.\
) else (
	echo call "%ProgramFiles%\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" UE4.sln  /p:Configuration="Development Editor" /p:Platform="Win64" /p:Project="UnrealBuildTool" /p:OutputPath=.\
	call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" UE4.sln  /p:Configuration="Development Editor" /p:Platform="Win64" /p:Project="UnrealBuildTool" /p:OutputPath=.\
)

:: TODO 判断 Build/engine/Engine/Binaries/Win64/ShaderCompileWorker.exe 是否存在来检查是否编译成功
echo Build success!

:: .\Engine\Binaries\Win64\UE4Editor.exe
