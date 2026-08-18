# EasyECS Runtime / Test

`EasyECS.sln`包含：

- `EasyECS`：Runtime静态库。
- `EasyECSTest`：Generator Smoke、功能测试、Benchmark和Fuzz。

## 下载后直接运行

`EasyECSTest/Data`已经提交完整generated代码，因此第一次下载仓库后可以直接打开：

```text
EasyECS.sln
```

将`EasyECSTest`设为启动项目后直接Build / F5即可，不要求先编译Generator。

`EasyECSTest`的Pre-Build会优先查找并调用：

```bat
EasyECSGenerator.exe --no-pause --scan Data
```

如果Generator尚未编译，只要仓库中提交的generated代码完整，Pre-Build会直接使用这些文件并继续编译；只有generated文件也缺失时才会失败。

测试项目提交的生成代码包括统一入口和所有分文件：

```text
Data/EasyECS.generated.h
Data/EasyECS.generated.cpp
Data/RoleData.easyecs.generated.h/.cpp
Data/CharacterData.easyecs.generated.h/.cpp
Data/ItemData.easyecs.generated.h/.cpp
Data/Battle/BulletData.easyecs.generated.h/.cpp
```

工程只编译`Data/EasyECS.generated.cpp`。各个`*.easyecs.generated.cpp`由统一入口包含，不要重复加入编译。

## 测试命令

```bat
EasyECSTest.exe smoke --no-pause
EasyECSTest.exe list --no-pause
EasyECSTest.exe dictionary --no-pause
EasyECSTest.exe fuzz --no-pause
EasyECSTest.exe all --no-pause
```

日常性能检查使用`list`/`dictionary`即可；`fuzz`单独执行百万次随机差分；发布前使用`all`。

功能测试或Fuzz出现失败时，程序返回非0退出码。
