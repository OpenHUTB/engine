:: call setEnv64.bat
:: %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

call Setup.bat

call GenerateProjectFiles.bat


:: %ProgramFiles(x86)%=C:\Program Files (x86)
:: %ProgramW6432%=C:\Program Files
:: call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" Engine\Intermediate\ProjectFiles\UE4.vcxproj

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\bin\MSBuild.exe" UE4.sln  /p:Configuration="Development Editor" /p:Platform="Win64" /p:Project="UnrealBuildTool"

echo Build success!

pause