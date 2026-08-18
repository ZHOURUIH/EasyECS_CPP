# EasyECSGenerator

打开`EasyECSGenerator.sln`编译独立Generator EXE。

推荐目录扫描模式：

```bat
EasyECSGenerator.exe --scan "C:\Project\Data"
```

自动构建时增加`--no-pause`：

```bat
EasyECSGenerator.exe --no-pause --scan "C:\Project\Data"
```

Generator递归扫描`.h/.hpp`，识别`ECS()`结构体，在源头文件旁生成对应`*.easyecs.generated.h/.cpp`，并在扫描根目录生成统一的`EasyECS.generated.h/.cpp`入口。

当前支持同文件多个ECS类型、多个头文件、namespace/嵌套namespace、alias、enum、custom trivially-copyable类型、`std::array`、顶层`const`字段和标准`[[...]]`属性等已验证类型。

生成内容没有变化时不会重写文件；源ECS文件删除后会自动清理对应的旧generated文件。
