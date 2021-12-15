## 介绍

**DexRewrite** 是一个针对Android dex文件分析、解析、重构的 C++ 库

## 如何使用

1. 使用 [Android Studio](https://developer.android.google.cn/studio/) 和 Cmake编译
2. 使用 [Jetbrains Clion](https://www.jetbrains.com/clion/) 和 Cmake编译

## 核心功能

* [X]  解析整个dex文件。
* [X]  统计字符串、函数、字段引用。
* [X]  反编译指定函数
* [X]  Android隐藏API调用扫描

## 待发布功能

* [X]  Dex文件合并（已完成，测试中，待开源）
* [X]  Dex ASM汇编支持（已经完成、测试中，待开源）

## 待修复BUG

* [ ]  Dex文件合并后，各个索引值排序问题
* [ ]  ASM 框架插入代码 goto、goto/16、goto/32 跳转存在问题

## 参考代码

相关使用可以参考 main.cpp repairdex.cpp文件

### 参考代码一

遍历整个dex文件中的所有类

```c++
for (const auto &class_data : mHeader->ClassDatas()) {
    const char *class_name = class_data->GetClassDef()->ClassType()->GetStringId()->Data();
    for (auto &direct_method : *class_data.get()->DirectMethods()) {
        repairNopCode(direct_method->GetCodeItem());
    }   
    for (auto &virtual_method: *class_data.get()->VirtualMethods()) {
        repairNopCode(virtual_method->GetCodeItem());
    }
}
```

### 参考代码二

遍历dex文件中所有函数实体

```c++
for (const auto &class_data : mHeader->ClassDatas()) {
    for (auto &direct_method : *class_data.get()->DirectMethods()) {
        repairNopCode(direct_method->GetCodeItem());
    }
    for (auto &virtual_method: *class_data.get()->VirtualMethods()) {
        repairNopCode(virtual_method->GetCodeItem());
    }
}
```

或者

```c++
for (const auto &methodItem : mHeader->MethodItems()) {
    auto index = methodItem.first;
    auto method_item = methodItem.second;
}

```

### 参考代码三

遍历所有Dex文件中的Codeitem实体

```c++

for (const auto &codeItem : mHeader->CodeItems()) {

}

```

### 参考代码四

将dex文件输出到文件

```C++
dex_ir::DexWriter::Output(mHeader.get(), "out.dex");
```

## 关于代码仓库

致力于脱离Java环境，实现一套可以对Dex文件反编译、字节码编译的框架。开源版本目前仅限于重写Dex文件、反编译Dex文件。

至于DexASM 和 DexMerger 相关代码已经实现，正在测试中，目前用于公司产品上，稳定后将会开源。

## 引用

[https://android.googlesource.com/platform/art/+/master/dexlayout/](https://android.googlesource.com/platform/art/+/master/dexlayout/)

[https://android.googlesource.com/platform/art/+/refs/heads/master/libdexfile/](https://android.googlesource.com/platform/art/+/refs/heads/master/libdexfile/)
