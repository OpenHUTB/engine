@echo off
setlocal
pushd "%~dp0"

rem Figure out if we should append the -prompt argument
set PROMPT_ARGUMENT=
for %%P in (%*) do if /I "%%P" == "--prompt" goto no_prompt_argument
for %%P in (%*) do if /I "%%P" == "--force" goto no_prompt_argument
set PROMPT_ARGUMENT=--prompt
:no_prompt_argument

rem 同步依赖...
.\Engine\Binaries\DotNET\GitDependencies.exe %PROMPT_ARGUMENT% %*
if ERRORLEVEL 1 goto error

rem Setup the git hooks...
if not exist .git\hooks goto no_git_hooks_directory
echo Registering git hooks...
echo #!/bin/sh >.git\hooks\post-checkout
echo Engine/Binaries/DotNET/GitDependencies.exe %* >>.git\hooks\post-checkout
echo #!/bin/sh >.git\hooks\post-merge
echo Engine/Binaries/DotNET/GitDependencies.exe %* >>.git\hooks\post-merge
:no_git_hooks_directory

rem 安装先决条件...
echo Installing prerequisites...
start /wait Engine\Extras\Redist\en-us\UE4PrereqSetup_x64.exe /quiet

rem 注册引擎安装...
if not exist .\Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe goto :no_unreal_version_selector
.\Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe /register
:no_unreal_version_selector

rem 完成!
goto :end

rem 发生错误，等待按键来退出。
:error
pause

:end
popd
