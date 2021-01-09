# DexRewrite

1. A Framework for Rewriting DEX Files，Mainly for Android shell unpack,
2. Find references to related fields
3. Simple decompilation of Dex file
4. ...
# How to build?

1. Build in android studio with cmake
2. Build in clion with cmake

# Usage

There are usage examples in main.cpp or repairdex.cpp

# eg.1

lookup all class

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

# eg.2

lookup all method

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

or

```c++
for (const auto &methodItem : mHeader->MethodItems()) {
    auto index = methodItem.first;
    auto method_item = methodItem.second;
}

```

# eg.3

lookup all codeitem

```c++

for (const auto &codeItem : mHeader->CodeItems()) {

}

```

# About

Modified from AOSP

[https://android.googlesource.com/platform/art/+/master/dexlayout/](https://android.googlesource.com/platform/art/+/master/dexlayout/)

[https://android.googlesource.com/platform/art/+/refs/heads/master/libdexfile/](https://android.googlesource.com/platform/art/+/refs/heads/master/libdexfile/)
