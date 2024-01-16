echo off

:: 将当前目录更改为自动化工具的 AutomationTool 文件夹 
cd Engine\Binaries\DotNET\AutomationTool

:: AutomationTool.exe是实际的自动化工具可执行文件，它会使用自动化工具和 BuildGraph 生成已安装的构建。
:: BuildGraph是生成已安装版本本身的命令
:: -target="Make Installed Build Win64" 指定 BuildGraph 将运行的目标。在这种情况下是 Win64，（例如还可能是 Mac）。
:: -script=Engine/Build/InstalledEngineBuild.xml是描述 BuildGraph 将用于生成已安装版本的所有默认参数的文件的路径。
AutomationTool.exe BuildGraph -target="Make Installed Build Win64" -script=Engine/Build/InstalledEngineBuild.xml -set:HostPlatformOnly=true

:: Just an empty line to separate the outputs.
echo.

:: Just a friendly reminder to the user that the process is done.
echo. Install Build Generate Script complete

:: The `pause` below prevents the window from closing immediately after the script finishes running.
pause