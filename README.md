# EasyECS C++

EasyECS C++是一个面向`struct`数据的AoS→SoA代码生成方案。业务代码继续定义普通C++结构体，Generator自动生成高性能`List`、`Dictionary`、Ref访问器和Direct Column接口。

当前版本已经完成核心功能、性能优化、Copy/Move验证以及1,000,000次随机差分/Fuzz测试，可以作为稳定使用基线。

## 特点

- 业务层仍然使用普通`struct`定义数据，不要求改成传统Archetype ECS写法。
- 默认字段自动拆成SoA Column，`NOT_ECS()`字段保留在AoS Block。
- 自动生成`ECSList`、`ECSDictionary`、`Ref/ConstRef`和Direct Column接口。
- Dictionary使用EasyECS自带的Flat IndexMap，不依赖`std::unordered_map`保存索引。
- 支持批量Add、Build、Remove、RemoveAll、Capacity管理、Clear复用、Copy/Move。
- Windows / Linux都只需要C++17。
- Generator只在生成内容变化时重写文件，并自动清理失效的generated文件。

## 目录结构

```text
EasyECS/
├─ Generator/
│  ├─ EasyECSGenerator.sln
│  └─ EasyECSGenerator/
└─ EasyECS/
   ├─ EasyECS.sln
   ├─ EasyECS/
   │  ├─ EasyECS.h
   │  └─ EasyECSIndexMap.h
   └─ EasyECSTest/
```

如果只是把EasyECS接入自己的工程，实际运行时只需要这2个Runtime文件：

```text
EasyECS.h
EasyECSIndexMap.h
```

再加上Generator生成的：

```text
EasyECS.generated.h
EasyECS.generated.cpp
```

工程中**只编译统一入口`EasyECS.generated.cpp`**，不要再单独编译各个`*.easyecs.generated.cpp`。

## 下载仓库后直接运行测试

`EasyECSTest/Data`已经提交了当前测试数据对应的**完整生成代码**，所以第一次下载仓库后不需要先编译Generator。

Windows下直接：

```text
1. 打开 EasyECS/EasyECS.sln
2. 将 EasyECSTest 设为启动项目
3. 选择 Debug x64 或 Release x64
4. Build / F5
```

测试工程中已经包含：

```text
Data/EasyECS.generated.h
Data/EasyECS.generated.cpp
Data/RoleData.easyecs.generated.h/.cpp
Data/CharacterData.easyecs.generated.h/.cpp
Data/ItemData.easyecs.generated.h/.cpp
Data/Battle/BulletData.easyecs.generated.h/.cpp
```

`EasyECSTest`的Pre-Build行为是：

```text
已存在EasyECSGenerator.exe  -> 自动重新生成测试代码
不存在EasyECSGenerator.exe  -> 直接使用仓库中已提交的生成代码
生成代码也不完整            -> 才会报错并要求先编译Generator
```
`注意:Data/EasyECS.generated.cpp已经不再自动生成,因为需要兼容cpp工程可能使用的UnityBuild,所以generate.cpp的编译需要自行根据自己的项目情况处理,示例测试项目中为了方便是带unitybuild文件的,也就是EasyECS.generated.cpp`

因此只是下载代码、查看Benchmark或运行测试时，不需要先处理Generator工程。只有修改了`Data`中的ECS定义并希望重新生成代码时，才需要先编译Generator。

---

# 快速开始

先定义普通数据：

```cpp
#pragma once
#include "EasyECS.h"

ECS()
struct RoleData
{
	int mHP = 0;
	float mSpeed = 0.0f;
	float mPositionX = 0.0f;
	float mPositionY = 0.0f;
	NOT_ECS() int mID = 0;
};
```

默认字段进入SoA：

```text
mHP
mSpeed
mPositionX
mPositionY
```

`NOT_ECS()`字段进入AoS Block：

```text
mID
```

执行Generator：

```text
EasyECSGenerator --scan Data目录
```

然后业务代码直接使用：

```cpp
#include "EasyECS.generated.h"

RoleDataECSList list;
RoleData role;
role.mHP = 100;
role.mSpeed = 5.0f;
list.add(role);

RoleDataRef ref = list[0];
ref.mHP += 10;
```

---

# Windows + Visual Studio接入

## 推荐方式：直接把Runtime源码加入自己的工程

这是最简单的接入方式，**不要求额外建立EasyECS静态库工程，也不要求修改现有Solution结构**。

推荐目录：

```text
YourProject/
├─ ThirdParty/
│  └─ EasyECS/                 ← EasyECS仓库
├─ YourGame/
│  ├─ Data/                    ← 放ECS()数据头文件
│  ├─ main.cpp
│  └─ YourGame.vcxproj
└─ YourProject.sln
```

## 第1步：编译Generator一次

用Visual Studio 2022打开：

```text
ThirdParty/EasyECS/Generator/EasyECSGenerator.sln
```

选择：

```text
Release | x64
```

编译后得到：

```text
ThirdParty/EasyECS/Generator/Bin/Release/EasyECSGenerator.exe
```

Generator一般只需要在Generator源码发生变化时重新编译。

## 第2步：把Runtime加入自己的VS工程

`EasyECS.h`和`EasyECSIndexMap.h`只需要能被Include，不需要单独编译。

在：

```text
项目属性 → C/C++ → 常规 → 附加包含目录
```

加入：

```text
$(SolutionDir)ThirdParty\EasyECS\EasyECS\EasyECS
$(ProjectDir)Data
```

在：

```text
项目属性 → C/C++ → 语言 → C++语言标准
```

选择：

```text
ISO C++17 (/std:c++17)
```

## 第3步：第一次生成代码

第一次需要先生成统一入口文件。按照上面的推荐目录，如果当前CMD位于`YourProject`根目录，直接执行：

```bat
ThirdParty\EasyECS\Generator\Bin\Release\EasyECSGenerator.exe --scan "YourGame\Data"
```

也可以直接使用绝对路径：

```bat
D:\Project\ThirdParty\EasyECS\Generator\Bin\Release\EasyECSGenerator.exe --scan "D:\Project\YourGame\Data"
```

上面的"YourGame\Data"表示生成器需要扫描的目录,仅在此目录中带有ECS()宏的结构体才会生成对应的ECSList

Generator会在`Data`目录生成：

```text
RoleData.easyecs.generated.h
RoleData.easyecs.generated.cpp
...
EasyECS.generated.h
```

这就是Windows下推荐的完整工作流。

## Windows最少需要记住的内容

```text
1. 编译一次EasyECSGenerator.exe
2. 执行Generator --scan 需要扫描的目录
```

---

# Linux接入

Linux下不依赖Visual Studio，也不需要Windows生成的EXE。Generator本身就是普通C++17程序，使用GCC或Clang编译一次即可。

假设目录仍然是：

```text
YourProject/
├─ ThirdParty/EasyECS/
├─ Data/
└─ main.cpp
```

## 第1步：编译Linux Generator一次

GCC：

```bash
mkdir -p ThirdParty/EasyECS/Generator/Bin/Linux

g++ -std=c++17 -O2 \
    -IThirdParty/EasyECS/Generator/EasyECSGenerator \
    ThirdParty/EasyECS/Generator/EasyECSGenerator/*.cpp \
    -o ThirdParty/EasyECS/Generator/Bin/Linux/EasyECSGenerator
```

也可以直接换成`clang++`：

```bash
clang++ -std=c++17 -O2 \
    -IThirdParty/EasyECS/Generator/EasyECSGenerator \
    ThirdParty/EasyECS/Generator/EasyECSGenerator/*.cpp \
    -o ThirdParty/EasyECS/Generator/Bin/Linux/EasyECSGenerator
```

## 第2步：生成代码

```bash
./ThirdParty/EasyECS/Generator/Bin/Linux/EasyECSGenerator --scan ./Data
```

## 第3步：编译自己的程序

最简单的GCC示例：

```bash
g++ -std=c++17 -O2 \
    -I./ThirdParty/EasyECS/EasyECS/EasyECS \
    -I./Data \
    ./main.cpp \
    -o ./app
```

## Linux自动生成

如果使用Makefile，最简单可以把Generator放在正常编译之前：

```makefile
EASY_ECS_ROOT := ThirdParty/EasyECS
ECS_DATA := Data
EASY_ECS_GENERATOR := $(EASY_ECS_ROOT)/Generator/Bin/Linux/EasyECSGenerator

.PHONY: easyecs

easyecs:
	$(EASY_ECS_GENERATOR) --scan $(ECS_DATA)

app: easyecs
	g++ -std=c++17 -O2 \
		-I$(EASY_ECS_ROOT)/EasyECS/EasyECS \
		-I$(ECS_DATA) \
		main.cpp \
		-o app
```

已有大型Make/CMake工程时，也只需要遵守同一原则：

```text
编译前运行Generator
```

不需要让构建系统知道每一个单独的generated文件。

---

# Generator

手动扫描：

```bat
EasyECSGenerator.exe --scan "C:\Project\Data"
```

自动Build：

```bat
EasyECSGenerator.exe --scan "C:\Project\Data"
```

Linux：

```bash
./EasyECSGenerator --scan ./Data
```

Generator递归扫描`.h/.hpp`，每个包含`ECS()`的源头文件旁生成：

```text
<Name>.easyecs.generated.h
<Name>.easyecs.generated.cpp
```

扫描根目录同时生成：

```text
EasyECS.generated.h
```

特性：

- 支持一个头文件多个ECS struct。
- 支持多个头文件和子目录。
- 支持namespace和嵌套namespace。
- 支持alias、enum、自定义trivially-copyable类型、`std::array`、顶层`const`字段、标准`[[...]]`属性等已验证类型。
- 生成内容未变化时不会重写文件。
- 源ECS文件删除后会自动清理对应旧generated文件。
- generated文件不应该手工修改。

---

# List API

```cpp
RoleDataECSList list;

list.add(value);
list.addRange(values, count);
RoleDataRef valueRef = list.addDefault();

list.insert(index, value);
list.removeAt(index);
list.removeAtSwapBack(index);
list.popBack();

list.removeAll([](RoleDataConstRef value)
{
	return value.mHP <= 0;
});

list.reserve(count);
list.shrinkToFit();

list.clear();
list.clearKeepCapacity();
list.clearAndRelease();
```

`clear()`与`clearKeepCapacity()`语义一致，适合频繁复用；只有明确希望归还内存时才使用`clearAndRelease()`。

---

# Ref与Direct Column

普通业务代码优先使用Ref：

```cpp
RoleDataRef role = list[i];
role.mHP += 1;
role.mPositionX += role.mSpeed;
```

极端热点循环优先Direct Column：

```cpp
int* EASY_ECS_RESTRICT hp = list.getHPColumn();
float* EASY_ECS_RESTRICT x = list.getPositionXColumn();
float* EASY_ECS_RESTRICT speed = list.getSpeedColumn();

for (int i = 0; i < list.size(); ++i)
{
	hp[i] += 1;
	x[i] += speed[i];
}
```

Benchmark中Direct已经基本达到Raw SoA性能。

注意：

> List/Dictionary发生扩容或结构变化后，不应该继续持有之前取得的Ref或Column指针。

---

# Dictionary API

```cpp
RoleDataECSDictionary<int> dictionary;

dictionary.add(key, value);
dictionary.tryAdd(key, value);
dictionary.set(key, value);

auto ref = dictionary.tryGetRef(key);
auto [valueRef, added] = dictionary.getOrAdd(key);

dictionary.remove(key);
dictionary.removeByIndex(index);
dictionary.removeBatch(keys);
dictionary.removeByIndexBatch(indices);

dictionary.removeAll([](const int& key, RoleDataConstRef value)
{
	return value.mHP <= 0;
});

dictionary.reserve(count);
dictionary.shrinkToFit();

dictionary.clear();
dictionary.clearKeepCapacity();
dictionary.clearAndRelease();
```

向已有Dictionary追加一批数据：

```cpp
int addedCount = dictionary.addRange(keys, values, count);
```

空Dictionary的一次性完整构建优先：

```cpp
bool success = dictionary.build(keys, values, count);
```

`build()`要求输入Key唯一；重复Key时返回`false`，Dictionary保持为空。

顺序遍历：

```cpp
dictionary.forEach([](const int& key, RoleDataRef value)
{
	value.mHP += 1;
});
```

或者直接按Dense Index访问：

```cpp
for (int i = 0; i < dictionary.size(); ++i)
{
	const int& key = dictionary.keyAt(i);
	RoleDataRef value = dictionary.valueAt(i);
}
```

---

# Copy / Move

生成的List、Dictionary以及底层`EasyECSIndexMap`都支持：

```cpp
T copy(source);
copy = source;

T moved(std::move(source));
moved = std::move(other);
```

Move后的源对象保持合法空状态，可以再次`add()`、`clear()`或正常析构。

---

# Benchmark

下面是当前稳定版本在**Windows / Visual Studio MSVC Release x64**下的实测结果。

测试参数：

```text
EntityCount:500000
StructuralCount:100000
SampleCount:15
WarmupCount:3
```

测试日期：`2026-08-18`。

> 不同CPU、内存、编译器版本会改变绝对耗时，因此`ms/ns`主要用于记录当前测试基线，实际更应该关注同一台机器上的相对倍率。

## Benchmark结果摘要

| 场景 | 结果 |
|---|---:|
| List addRange vs repeated add | **1.30x** |
| List removeAll vs vector erase/remove_if | **1.85x** |
| 1字段 Direct vs vector | **11.17x** |
| IndexMap Find vs unordered_map | **2.32x** |
| IndexMap Add vs unordered_map | **4.20x** |
| IndexMap Remove vs unordered_map | **5.36x** |
| Dictionary随机修改1字段 IndexDirect vs unordered_map | **3.04x** |
| Dictionary顺序修改1字段 Direct vs unordered_map | **517.92x** |
| Dictionary build vs repeated add + reserve | **1.26x** |
| Dictionary removeAll vs unordered_map predicate erase | **2.76x** |
| 80% removeByIndexBatch vs repeated remove | **3.30x** |
| IndexMap长期Churn vs unordered_map | **1.65x** |

## List批量构建

| 实现 | Median | Min | Max | ns/entity |
|---|---:|---:|---:|---:|
| Repeated add | 1.399 ms | 1.327 | 1.501 | 2.798 |
| addRange Row-wise | **1.079 ms** | 0.994 | 1.189 | **2.159** |

```text
Repeated / Row-wise: 1.30x
```

## List条件删除50%

| 实现 | Median | Min | Max | ns/entity |
|---|---:|---:|---:|---:|
| vector erase/remove_if | 1.040 ms | 0.744 | 1.692 | 2.080 |
| ECS removeAll | **0.563 ms** | 0.500 | 0.747 | **1.126** |

```text
Vector / ECS: 1.85x
```

## List修改1个字段

| 实现 | Median | Min | Max | ns/entity |
|---|---:|---:|---:|---:|
| vector<RoleData> | 0.277 ms | 0.272 | 0.283 | 0.554 |
| RoleData[] | 0.285 ms | 0.273 | 0.323 | 0.570 |
| ECS list[i] | 0.102 ms | 0.098 | 0.132 | 0.203 |
| ECS Ref | 0.087 ms | 0.086 | 0.088 | 0.175 |
| ECS Direct | **0.025 ms** | 0.024 | 0.029 | 0.050 |
| Raw SoA | **0.025 ms** | 0.023 | 0.064 | 0.049 |

```text
Index / Direct  : 4.10x
Ref / Direct    : 3.52x
Vector / Direct : 11.17x
```

## List访问2个字段

| 实现 | Median | Min | Max | ns/entity |
|---|---:|---:|---:|---:|
| vector<RoleData> | 0.279 ms | 0.273 | 0.302 | 0.558 |
| RoleData[] | 0.278 ms | 0.273 | 0.315 | 0.556 |
| ECS list[i] | 0.153 ms | 0.150 | 0.154 | 0.306 |
| ECS Ref | 0.214 ms | 0.209 | 0.391 | 0.428 |
| ECS Direct | **0.067 ms** | 0.067 | 0.067 | **0.134** |
| Raw SoA | 0.070 ms | 0.069 | 0.070 | 0.139 |

```text
Index / Direct  : 2.29x
Ref / Direct    : 3.20x
Vector / Direct : 4.17x
```

## List访问4个字段

| 实现 | Median | Min | Max | ns/entity |
|---|---:|---:|---:|---:|
| vector<RoleData> | 0.376 ms | 0.363 | 0.414 | 0.751 |
| RoleData[] | 0.388 ms | 0.351 | 0.418 | 0.775 |
| ECS list[i] | 0.561 ms | 0.545 | 0.604 | 1.122 |
| ECS Ref | 0.487 ms | 0.478 | 0.571 | 0.975 |
| ECS Direct | 0.155 ms | 0.146 | 0.237 | 0.310 |
| Raw SoA | **0.154 ms** | 0.153 | 0.157 | **0.308** |

```text
Index / Direct  : 3.63x
Ref / Direct    : 3.15x
Vector / Direct : 2.43x
```

## List只读4个字段

| 实现 | Median | Min | Max | ns/entity |
|---|---:|---:|---:|---:|
| vector<RoleData> | 0.404 ms | 0.399 | 0.412 | 0.808 |
| RoleData[] | 0.380 ms | 0.372 | 0.411 | 0.760 |
| ECS Ref | 0.387 ms | 0.373 | 0.401 | 0.773 |
| ECS Direct | 0.384 ms | 0.368 | 0.430 | 0.768 |
| Raw SoA | **0.362 ms** | 0.361 | 0.382 | **0.723** |

只读多个字段时，Ref/Direct/Raw SoA已经处于相同量级，AoS与SoA的差异会明显小于只访问少量字段的场景。

## EasyECSIndexMap内存

```text
Memory:12.000 MB
Capacity:1048576
Tombstone:0
AvgFindProbe:1.458
MaxFindProbe:25
```

## IndexMap随机Find

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map<int,int> | 7.947 ms | 6.039 | 11.194 | 15.893 |
| EasyECSIndexMap<int> | **3.431 ms** | 2.700 | 5.373 | **6.861** |

```text
unordered_map / EasyECSIndexMap: 2.32x
```

## IndexMap Add

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map<int,int> | 4.120 ms | 3.821 | 6.305 | 41.201 |
| EasyECSIndexMap<int> | **0.981 ms** | 0.917 | 1.219 | **9.811** |

```text
unordered_map / EasyECSIndexMap: 4.20x
```

## IndexMap Remove

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map<int,int> | 2.659 ms | 2.581 | 3.743 | 26.592 |
| EasyECSIndexMap<int> | **0.496 ms** | 0.465 | 0.708 | **4.964** |

```text
unordered_map / EasyECSIndexMap: 5.36x
```

## Dictionary按Key随机修改1个字段

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map<int, RoleData> | 13.070 ms | 12.335 | 14.893 | 26.140 |
| ECS Dictionary Ref | 4.592 ms | 3.906 | 6.229 | 9.183 |
| ECS Dictionary IndexDirect | **4.299 ms** | 3.875 | 6.008 | **8.597** |

```text
unordered_map / ECS Ref         : 2.85x
unordered_map / ECS IndexDirect : 3.04x
```

## Dictionary按Key随机访问2个字段

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map<int, RoleData> | 12.090 ms | 11.494 | 13.395 | 24.180 |
| ECS Dictionary Ref | 4.832 ms | 4.337 | 6.799 | 9.665 |
| ECS Dictionary IndexDirect | **4.332 ms** | 3.824 | 5.439 | **8.664** |

```text
unordered_map / ECS Ref         : 2.50x
unordered_map / ECS IndexDirect : 2.79x
```

## Dictionary顺序遍历修改1个字段

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map iteration | 11.550 ms | 9.652 | 13.789 | 23.099 |
| ECS Dictionary Ref | 0.152 ms | 0.152 | 0.153 | 0.305 |
| ECS Dictionary Direct | **0.022 ms** | 0.022 | 0.027 | **0.045** |

```text
unordered_map / ECS Ref    : 75.78x
unordered_map / ECS Direct : 517.92x
```

这是SoA连续Column在热点顺序循环中的主要使用场景。

## Dictionary顺序遍历访问4个字段

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map iteration | 9.408 ms | 8.019 | 10.832 | 18.815 |
| ECS Dictionary Ref | 0.404 ms | 0.404 | 0.413 | 0.808 |
| ECS Dictionary Direct | **0.142 ms** | 0.141 | 0.146 | **0.284** |

```text
unordered_map / ECS Ref    : 23.30x
unordered_map / ECS Direct : 66.30x
```

## Dictionary顺序只读4个字段

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map iteration | 9.355 ms | 8.251 | 13.678 | 18.711 |
| ECS Dictionary Ref | 0.367 ms | 0.357 | 0.393 | 0.734 |
| ECS Dictionary Direct | **0.356 ms** | 0.352 | 0.364 | **0.712** |

## Dictionary Add

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map Add | 4.558 ms | 4.087 | 6.169 | 45.575 |
| ECS Dictionary Add | **1.959 ms** | 1.863 | 2.121 | **19.591** |

```text
ECS / unordered_map: 0.43x
约等于unordered_map耗时是ECS的2.33x
```

## Dictionary批量构建

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| Repeated add + reserve | 1.836 ms | 1.739 | 2.097 | 18.356 |
| Dictionary addRange | 1.804 ms | 1.756 | 1.948 | 18.042 |
| Dictionary build | **1.457 ms** | 1.363 | 1.581 | **14.567** |

```text
Repeated / addRange : 1.02x
Repeated / build    : 1.26x
addRange / build    : 1.24x
```

空Dictionary已有完整输入数据时，推荐`build()`。

## Dictionary Clear后复用

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| ClearKeepCapacity + Rebuild | **1.221 ms** | 1.155 | 2.363 | **12.214** |
| ClearAndRelease + Rebuild | 3.744 ms | 3.434 | 4.500 | 37.442 |

```text
Release / KeepCapacity: 3.07x
```

因此频繁复用时默认使用`clear()`/`clearKeepCapacity()`，不要反复释放容量。

## Dictionary Remove

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map Remove | 2.959 ms | 2.765 | 4.786 | 29.591 |
| ECS Dictionary Remove | **1.106 ms** | 1.077 | 1.309 | **11.064** |

```text
ECS / unordered_map: 0.37x
约等于unordered_map耗时是ECS的2.68x
```

## Dictionary条件删除50%

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map predicate erase | 1.199 ms | 1.081 | 1.428 | 11.991 |
| ECS Dictionary removeAll | **0.434 ms** | 0.430 | 0.665 | **4.339** |

```text
unordered_map / ECS: 2.76x
```

## Dictionary Batch Remove 50%

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| ECS Repeated Remove 50% | 0.690 ms | 0.645 | 1.395 | 13.802 |
| ECS removeBatch 50% | 0.639 ms | 0.625 | 0.717 | 12.780 |
| ECS removeByIndexBatch 50% | **0.396 ms** | 0.389 | 0.775 | **7.928** |

```text
Repeated / removeBatch : 1.08x
Repeated / IndexBatch  : 1.74x
```

## Dictionary Batch Remove 80%

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| ECS Repeated Remove 80% | 1.086 ms | 1.007 | 1.601 | 13.572 |
| ECS removeBatch 80% | 0.616 ms | 0.610 | 0.699 | 7.695 |
| ECS removeByIndexBatch 80% | **0.329 ms** | 0.307 | 0.363 | **4.111** |

```text
Repeated / removeBatch : 1.76x
Repeated / IndexBatch  : 3.30x
```

## IndexMap长期Churn

参数：

```text
ChurnEntityCount:100000
BatchCount:10000
RoundCount:50
```

| 实现 | Median | Min | Max | ns/op |
|---|---:|---:|---:|---:|
| unordered_map Churn | 27.130 ms | 25.956 | 29.051 | 27.130 |
| EasyECSIndexMap Churn | **16.432 ms** | 15.705 | 17.625 | **16.432** |

```text
unordered_map / EasyECSIndexMap: 1.65x
```

最终Probe状态：

```text
AvgFindProbe:1.255
MaxFindProbe:17
AvgInsertProbe:2.395
MaxInsertProbe:46
TombstoneProbe:263962
Rehash:5
FinalTombstone:41435
Capacity:262144
```

---

# Fuzz / 正确性验证

Dictionary已经进行固定随机种子的1,000,000次随机差分测试：

```text
TotalOperations:1000000
ValidateInterval:64

NormalHash Fuzz Test    :PASS | Ops:600000 | KeySpace:1024 | FullChecks:233624 | FinalSize:6
CollisionHash Fuzz Test :PASS | Ops:300000 | KeySpace:256  | FullChecks:116882 | FinalSize:13
ConstantHash Fuzz Test  :PASS | Ops:100000 | KeySpace:64   | FullChecks:38817  | FinalSize:5

Dictionary Fuzz Test:PASS
```

其中ConstantHash会让所有Key产生相同Hash，用于持续压测：

```text
Collision
Linear Probe
Tombstone
Rehash
Dense Index
IndexToSlot
Batch Remove
Compact
Copy / Move
```

随机操作覆盖：

```text
add
tryAdd
set
getOrAdd
remove
removeByIndex
removeBatch
removeByIndexBatch
removeAll
addRange
build
reserve
shrinkToFit
clearKeepCapacity
clearAndRelease
copy constructor
copy assignment
move constructor
move assignment
tryGetRef
```

---

# 测试命令

Windows：

```bat
EasyECSTest.exe smoke
EasyECSTest.exe list
EasyECSTest.exe dictionary
EasyECSTest.exe fuzz
EasyECSTest.exe all
```

含义：

- `smoke`：Generator与生成代码功能验证。
- `list`：List长期Benchmark + List功能测试。
- `dictionary`：Dictionary/IndexMap长期Benchmark + 功能测试，不跑百万次Fuzz。
- `fuzz`：只运行Dictionary随机差分/Fuzz。
- `all`：发布前全量验证，最后运行Fuzz。

功能测试或Fuzz出现`FAILED`时程序返回非0退出码，可以直接接入CI。

---

# 使用建议

## 高频创建后复用

优先：

```cpp
container.clear();
// 或
container.clearKeepCapacity();
```

只有明确要归还内存才使用：

```cpp
container.clearAndRelease();
```

## 空Dictionary批量构建

优先：

```cpp
dictionary.build(keys, values, count);
```

## 已有Dictionary追加数据

使用：

```cpp
dictionary.addRange(keys, values, count);
```

## 热点循环

极端热点优先：

```cpp
getXXXColumn()
```

普通业务优先：

```cpp
Ref / ConstRef
```

## 批量删除

已经知道Dense Index时优先：

```cpp
removeByIndexBatch()
```

按Key批量删除：

```cpp
removeBatch()
```

按条件批量删除：

```cpp
removeAll(predicate)
```

---

# 当前稳定策略

当前核心能力已经完成并经过Benchmark、功能测试、碰撞测试、长期Churn和百万次Fuzz验证。

后续不应因为单次Benchmark波动随意修改这些已经稳定的核心策略：

```text
List SoA存储布局
Direct Column访问方式
IndexMap Control Byte + Hash Tag
线性Probe方案
正常增长75%负载因子
shrinkToFit约50%目标负载
Batch Remove当前策略
```

如果以后增加Serialization、Parallel View、Sort/Reorder、更复杂Parser等大功能，建议进入新的版本线，而不是继续改变当前稳定基线。

---

# 代码格式与仓库约定

仓库根目录提供`.editorconfig`和`.clang-format`，用于统一Windows、Linux、Visual Studio、VS Code、CLion等环境中的基础格式。

当前约定：

```text
C++标准            C++17
C++缩进            Tab
Tab宽度            4
花括号              Allman
普通源码建议行宽    <= 160
C++源码编码         UTF-8
C++源码换行         LF
.bat换行            CRLF
.sln换行            CRLF
文件末尾            保留换行
行尾空格            禁止
```

简单getter、简单guard和短小内联函数允许保持单行；复杂条件、长参数列表和多步骤函数应在可读边界处换行，不为了格式化而制造大量无意义空行。

Generator生成的代码遵循同样的缩进、空格和换行规则。不要手工修改`*.easyecs.generated.h/.cpp`或`EasyECS.generated.h/.cpp`，需要调整生成代码格式时应修改Generator。

普通业务工程中的generated文件默认不建议提交到Git：

```text
*.easyecs.generated.h
*.easyecs.generated.cpp
EasyECS.generated.h
EasyECS.generated.cpp
```

但本仓库的`EasyECS/EasyECSTest/Data`是例外：**测试项目的完整generated代码会提交到仓库**，用于保证别人下载后即使还没有编译Generator，也能直接编译和运行`EasyECSTest`。`.gitignore`已经为这个目录配置了例外规则。

始终不提交的本机构建文件包括：

```text
*.user
.vs/
Bin/
Intermediate/
```

业务项目仍建议在构建前由Generator自动刷新generated文件；测试项目则同时保留一份已验证的generated基线作为开箱即用的源码。
