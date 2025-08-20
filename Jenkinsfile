pipeline {
    agent none

    stages
    {
        stage('windows')
        {
            // 调用时候需要知道本地节点的名字，否则会报错：‘Jenkins’ doesn’t have label ‘windows’
            // 在 Dashboard -> Manager Jenkins -> Nodes(http://172.21.108.56:8080/manage/computer/) 中进行配置
            agent any // { label "windows" }
            environment
            {
                UE4_ROOT = 'C:\\workspace\\UnrealEngine'
            }
            stages
            {
              
                // 初始化环境
                stage('windows setup')
                {
                    steps
                    {
                        bat """
                            call setEnv64.bat
                            call Setup.bat
                            call GenerateProjectFiles.bat
                        """
                    }
                }

                // 编译虚幻引擎源代码工程 UE4.sln
                // "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" UE4.sln /Build Release /Out log.txt
                // "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" Engine\Intermediate\ProjectFiles\UE4.vcxproj
                // 查找MSBuild.exe所在的路径 https://github.com/Microsoft/vswhere/wiki/Find-MSBuild
                stage('windows build')
                {
                    steps
                    {
                        bat """
                            call setEnv64.bat
                            MSBuild.exe Engine/Intermediate/ProjectFiles/UE4.vcxproj
                        """
                    }
                }

                stage('windows retrieve content')
                {
                    steps
                    {
                        bat """
                            call setEnv64.bat
                        """
                    }
                }

                stage('windows package')
                {
                    steps
                    {
                        bat """
                            call setEnv64.bat
                        """
                    }
                }

                // 需要配置亚马逊云的ID和访问密钥
                /*
                stage('windows deploy')
                {
                    // when { anyOf { branch "make_deploy_dev"; buildingTag() } }
                    steps {
                        bat """
                            call setEnv64.bat
                            git checkout .
                            make deploy ARGS="--replace-latest"
                        """
                    }
                }
                */

                // 发布 PythonAPI 到 pypi
                // 需要删除 *.egg 文件
                // 测试：make deploy ARGS="--deploy-to-pypi"

            }

            /*
            post
            {
                always
                {
                    deleteDir()
                }
            }
            */

        }
    }
    
}