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
rem .\Engine\Binaries\DotNET\GitDependencies.exe 使用方法:
rem    GitDependencies [选项]
rem 
rem 选项:
rem    --all                         同步所有目录Sync all folders
rem    --include=<X>                 Include binaries in folders called <X>
rem    --exclude=<X>                 Exclude binaries in folders called <X>
rem    --prompt                      Prompt before overwriting modified files
rem    --force                       Always overwrite modified files
rem    --root=<PATH>                 Set the repository directory to be sync
rem    --threads=<N>                 Use N threads when downloading new files
rem    --dry-run                     Print a list of outdated files and exit
rem    --max-retries                 Override maximum number of retries per file
rem    --proxy=<user:password@url>   Sets the HTTP proxy address and credentials
rem    --cache=<PATH>                Specifies a custom path for the download cache
rem    --cache-size-multiplier=<N>   Cache size as multiplier of current download
rem    --cache-days=<N>              Number of days to keep entries in the cache
rem    --no-cache                    Disable caching of downloaded files
rem 
rem Detected settings:
rem    Excluded folders: none
rem    Proxy server: none
rem    Download cache: D:\hutb\Build\engine\.git\ue4-gitdeps
rem 
rem Default arguments can be set through the UE4_GITDEPS_ARGS environment variable.
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

rem 注册引擎安装...
if not exist .\Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe goto :no_unreal_version_selector
:: fix build blocked by UnrealVersionSelector using slient and before prerequisites installation
:: reference: https://forums.unrealengine.com/t/silent-build-blocked-by-unrealversionselector-on-windows/271955/2
.\Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe /register /unattended
:no_unreal_version_selector

rem 安装先决条件...
echo Installing prerequisites...
start /wait Engine\Extras\Redist\en-us\UE4PrereqSetup_x64.exe /quiet
echo Prerequisites installed.

rem 完成!
goto :end

rem 发生错误，等待按键来退出。
:error
pause

:end
popd
