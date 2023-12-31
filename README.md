包含 CARLA 补丁的虚幻引擎
=============

## 构建步骤
1. 在终端中，导航到要保存虚幻引擎的位置并克隆虚幻引擎仓库：
```shell
git clone https://github.com/OpenHUTB/unreal.git
```
注意：尽可能使虚幻引擎文件夹靠近 C:\\，因为如果路径超过一定长度，则会在步骤 3 中的 Setup.bat 返回错误。

2. 运行配置脚本：
```shell
Setup.bat
GenerateProjectFiles.bat
```
    
3. 编译修改后的引擎：
使用 Visual Studio 2019 打开源文件夹内的文件 `UE4.sln`。
在构建栏中，确保已选择`Development Editor`、`Win64`和`UnrealBuildTool`选项。如果需要任何帮助，请查看本指南。
在解决方案资源管理器中，右键单击 `UE4` 并选择`生成`（`Build`）。


4.编译解决方案后，可以打开引擎，通过启动可执行文件 `Engine\Binaries\Win64\UE4Editor.exe` 来检查所有内容是否已正确安装。

笔记：如果安装成功，虚幻引擎的版本选择器应该能够识别。可以通过右键单击任何 `.uproject` 文件并选择 `Switch Unreal Engine version` 来检查这一点。应该会看到一个弹出窗口，显示`Source Build at PATH`，这里 PATH 是选择的安装路径。如果您在右键单击文件 `.uproject` 时看不到此选择器 `Generate Visual Studio project files`，则虚幻引擎安装出现问题，可能需要重新正确安装。

重要：到目前为止发生了很多事情。强烈建议在继续之前重新启动计算机。

5. （可选）运行编辑器

将启动项目设置为 UE4。

<img src=https://docs.unrealengine.com/4.26/Images/ProductionPipelines/DevelopmentSetup/BuildingUnrealEngine/SetUE4_StartPrj.webp alt="编辑页面" width="480" />


右键单击 UE4 项目，将鼠标悬停于"Debug" 上，然后 单击"启动新实例（Start New Instance）" 以启动编辑器（或者，你可以按键盘上的 F5键 来启动编辑器的新实例）。

<img src=https://docs.unrealengine.com/4.26/Images/ProductionPipelines/DevelopmentSetup/BuildingUnrealEngine/RunCompiledWindowsEditor.webp width="480" />



## 内容
虚幻引擎源码的构成。

### 编译
[编译配置参考](https://docs.unrealengine.com/4.26/zh-CN/ProductionPipelines/DevelopmentSetup/BuildConfigurations/)

### 像素流插件
相关源码位于：`unreal\Engine\Plugins\Media\PixelStreaming\PixelStreaming.uplugin`


## 官方指南
From this repository you can build the Unreal Editor for Windows, Mac and Linux, compile Unreal Engine games for Android, iOS, PlayStation 4, Xbox One and HTML5,
and build tools like Unreal Lightmass and Unreal Frontend. Modify them in any way you can imagine, and share your changes with others! 

We have a heap of documentation available for the engine on the web. If you're looking for the answer to something, you may want to start here: 

* [Unreal Engine Programming Guide](https://docs.unrealengine.com/latest/INT/Programming/index.html)
* [Unreal Engine API Reference](https://docs.unrealengine.com/latest/INT/API/index.html)
* [Engine source and GitHub on the Unreal Engine forums](https://forums.unrealengine.com/forumdisplay.php?1-Development-Discussion)

If you need more, just ask! A lot of Epic developers hang out on the [forums](https://forums.unrealengine.com/) or [AnswerHub](https://answers.unrealengine.com/), 
and we're proud to be part of a well-meaning, friendly and welcoming community of thousands. 


## 分支


We publish source for the engine in several branches:

The **[release branch](https://github.com/EpicGames/UnrealEngine/tree/release)** is extensively tested by our QA team and makes a great starting point for learning the engine or
making your own games. We work hard to make releases stable and reliable, and aim to publish new releases every few months.

The **[master branch](https://github.com/EpicGames/UnrealEngine/tree/master)** is the hub of changes from all our specialized engine development teams. Our internal game teams typically take engine snapshots from here, but it isn't subject to as much testing as release branches.

Individual teams have their own **development branches** for day to day work ([dev-core](https://github.com/EpicGames/UnrealEngine/tree/dev-core), [dev-mobile](https://github.com/EpicGames/UnrealEngine/tree/dev-mobile) and [dev-sequencer](https://github.com/EpicGames/UnrealEngine/tree/dev-sequencer), for example). These branches reflect the cutting edge of the engine and may be buggy - they may not even compile. Battle-hardened developers eager to test new features or work lock-step with us should head to one of these. We aim to merge development branches to master every 3-4 weeks.

Other short-lived branches may pop-up from time to time as we stabilize new releases or hotfixes.


## 开始并运行

The steps below will take you through cloning your own private fork, then compiling and running the editor yourself:

### Windows

1. Install **[GitHub for Windows](https://windows.github.com/)** then **[fork and clone our repository](https://guides.github.com/activities/forking/)**. 
   To use Git from the command line, see the [Setting up Git](https://help.github.com/articles/set-up-git/) and [Fork a Repo](https://help.github.com/articles/fork-a-repo/) articles.

   If you'd prefer not to use Git, you can get the source with the 'Download ZIP' button on the right. The built-in Windows zip utility will mark the contents of zip files 
   downloaded from the Internet as unsafe to execute, so right-click the zip file and select 'Properties...' and 'Unblock' before decompressing it. Third-party zip utilities don't normally do this.

1. Install **Visual Studio 2017**. 
   All desktop editions of Visual Studio 2017 can build UE4, including [Visual Studio Community 2017](http://www.visualstudio.com/products/visual-studio-community-vs), which is free for small teams and individual developers.
   To install the correct components for UE4 development, check the "Game Development with C++" workload, and the "Unreal Engine Installer" and "Nuget Package Manager" optional components.
  
1. Open your source folder in Explorer and run **Setup.bat**. 
   This will download binary content for the engine, as well as installing prerequisites and setting up Unreal file associations. 
   On Windows 8, a warning from SmartScreen may appear.  Click "More info", then "Run anyway" to continue.
   
   A clean download of the engine binaries is currently 3-4gb, which may take some time to complete.
   Subsequent checkouts only require incremental downloads and will be much quicker.
 
1. Run **GenerateProjectFiles.bat** to create project files for the engine. It should take less than a minute to complete.  

1. Load the project into Visual Studio by double-clicking on the **UE4.sln** file. Set your solution configuration to **Development Editor** and your solution
   platform to **Win64**, then right click on the **UE4** target and select **Build**. It may take anywhere between 10 and 40 minutes to finish compiling, depending on your system specs.

1. After compiling finishes, you can load the editor from Visual Studio by setting your startup project to **UE4** and pressing **F5** to debug.




### Mac
   
1. Install **[GitHub for Mac](https://mac.github.com/)** then **[fork and clone our repository](https://guides.github.com/activities/forking/)**. 
   To use Git from the Terminal, see the [Setting up Git](https://help.github.com/articles/set-up-git/) and [Fork a Repo](https://help.github.com/articles/fork-a-repo/) articles.
   If you'd rather not use Git, use the 'Download ZIP' button on the right to get the source directly.

1. Install the latest version of [Xcode](https://itunes.apple.com/us/app/xcode/id497799835).

1. Open your source folder in Finder and double-click on **Setup.command** to download binary content for the engine. You can close the Terminal window afterwards.

   If you downloaded the source as a .zip file, you may see a warning about it being from an unidentified developer (because .zip files on GitHub aren't digitally signed).
   To work around it, right-click on Setup.command, select Open, then click the Open button.

1. In the same folder, double-click **GenerateProjectFiles.command**.  It should take less than a minute to complete.  

1. Load the project into Xcode by double-clicking on the **UE4.xcworkspace** file. Select the **ShaderCompileWorker** for **My Mac** target in the title bar,
   then select the 'Product > Build' menu item. When Xcode finishes building, do the same for the **UE4** for **My Mac** target. Compiling may take anywhere between 15 and 40 minutes, depending on your system specs.
   
1. After compiling finishes, select the 'Product > Run' menu item to load the editor.




### Linux

1. [Set up Git](https://help.github.com/articles/set-up-git/) and [fork our repository](https://help.github.com/articles/fork-a-repo/).
   If you'd prefer not to use Git, use the 'Download ZIP' button on the right to get the source as a zip file.

1. Open your source folder and run **Setup.sh** to download binary content for the engine.

1. Both cross-compiling and native builds are supported. 

   **Cross-compiling** is handy when you are a Windows (Mac support planned too) developer who wants to package your game for Linux with minimal hassle, and it requires a [cross-compiler toolchain](http://cdn.unrealengine.com/CrossToolchain_Linux/v11_clang-5.0.0-centos7.zip) to be installed (see the [Linux cross-compiling page on the wiki](https://docs.unrealengine.com/latest/INT/Platforms/Linux/GettingStarted/)).

   **Native compilation** is discussed in [a separate README](Engine/Build/BatchFiles/Linux/README.md) and [community wiki page](https://www.ue4community.wiki/Legacy/Building_On_Linux). 




### 额外的目标平台

**Android** support will be downloaded by the setup script if you have the Android NDK installed. See the [Android getting started guide](https://docs.unrealengine.com/latest/INT/Platforms/Android/GettingStarted/).

**iOS** programming requires a Mac. Instructions are in the [iOS getting started guide](https://docs.unrealengine.com/latest/INT/Platforms/iOS/GettingStarted/index.html).

**HTML5** support will be downloaded by the setup script if you have Emscripten installed. Please see the [HTML5 getting started guide](https://docs.unrealengine.com/latest/INT/Platforms/HTML5/GettingStarted/index.html).

**PlayStation 4** or **Xbox One** development require additional files that can only be provided after your registered developer status is confirmed by Sony or Microsoft. See [the announcement blog post](https://www.unrealengine.com/blog/playstation-4-and-xbox-one-now-supported) for more information.


## 额外的笔记

The first time you start the editor from a fresh source build, you may experience long load times. 
The engine is optimizing content for your platform to the _derived data cache_, and it should only happen once.

Your private forks of the Unreal Engine code are associated with your GitHub account permissions.
If you unsubscribe or switch GitHub user names, you'll need to re-fork and upload your changes from a local copy. 

## 问题
* 右键`.uproject`文件没有`Switch Unreal Engine version...`
解决：双击`Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe`，出现`Register this directory as an Unreal Engine installation?`后点击`是(Y)`。

* 增加`matlab`插件进行虚幻引擎编译，导致启动虚幻编辑器启动失败，原因不明。


## 参考链接
[UE4初学者系列教程合集-全中文新手入门教程](https://www.bilibili.com/video/BV164411Y732/?share_source=copy_web&vd_source=d956d8d73965ffb619958f94872d7c57  )

[ue4官方文档](https://docs.unrealengine.com/4.26/zh-CN/)

[官方讨论社区](https://forums.unrealengine.com/categories?tag=unreal-engine)

[知乎的虚幻引擎社区](https://zhuanlan.zhihu.com/egc-community)
