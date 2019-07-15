# DexRewrite
A Framework for Rewriting DEX Files，Mainly for Android shell unpack,
# How to build?
1. Build in android studio with cmake
2. Build in clion with cmake
# Usage
There are usage examples in library.cpp
# eg.1
lookup all class

```c++
void lookup_all_class(dex_ir::Header *mHeader) {
    for (auto &class_item : mHeader->GetCollections().ClassDefs()) {
    }
}
```

# eg.2
lookup all dexcode，you can modify dexcode.

```c++
/**
 * look up all dexcode item
 * @param mHeader
 */
void lookup_all_code(dex_ir::Header *mHeader) {
    for (auto &code_item : mHeader->GetCollections().CodeItems()) {

    }
}

```

# eg.3
lookup all method

```c++

void lookup_all_method(dex_ir::Header *mHeader) {
    for (auto &method_item : mHeader->GetCollections().MethodItems()) {
        //name
        auto method_name = method_item.second->GetMethodId()->Name()->Data();
        auto method_sign = method_item.second->GetSignure();
        auto class_name = method_item.second->GetClassName();

        //dex_code
        auto code_item = method_item.second->GetCodeItem();
        if (code_item != nullptr) {

        }
        //fix 3
        // memcpy(method->GetCodeItem()->Insns(), dexcode_i1, method->GetCodeItem()->InsnsSize() * 2);
        //create new code_item
        //dex_ir::CodeItem *codeItem = mHeader->GetCollections().CreateCodeItem(*mDexFile.get(),reinterpret_cast<uint8_t *>(dexCode),index,code_items->GetMethodItem()->GetMethodId()->GetIndex());
        //code_items->GetMethodItem()->SetCodeItem(codeItem);
        // change AccessFlags to remove native falgs
        // method_item.second->SetAccessFlags()
    }
}

```
# About
Copy from AOSP
[https://android.googlesource.com/platform/art/+/master/dexlayout/](https://android.googlesource.com/platform/art/+/master/dexlayout/)
[https://android.googlesource.com/platform/art/+/refs/heads/master/libdexfile/](https://android.googlesource.com/platform/art/+/refs/heads/master/libdexfile/)
