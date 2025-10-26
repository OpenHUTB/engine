@echo off
setlocal
chcp 65001


:: call setEnv64.bat
%comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

:: 使用 --force 选项来跳过: Checking dependencies... overwrite your changes (y/n)
call Setup.bat --force

call GenerateProjectFiles.bat


:: %ProgramFiles(x86)%=C:\Program Files (x86)
:: %ProgramW6432%=C:\Program Files

:: 没有构建 UE4Editor.exe
:: call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" Engine\Intermediate\ProjectFiles\UE4.vcxproj

:: 注意：双引号必须且只能将包含空格的目录
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" UE4.sln  /p:Configuration="Development Editor" /p:Platform="Win64" /p:Project="UnrealBuildTool"

echo Build success!

:: .\Engine\Binaries\Win64\UE4Editor.exe
