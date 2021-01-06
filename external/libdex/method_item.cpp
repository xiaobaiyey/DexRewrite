//
// Created by xiaob on 2020/1/1.
//
#include <iostream>
#include "method_item.h"
#include "class_data.h"
#include "class_def.h"

namespace dex_ir {

    uint32_t MethodItem::GetAccessFlags() const {
        return access_flags_;
    }

    const MethodId *MethodItem::GetMethodId() const {
        return method_id_;
    }

    void MethodItem::SetCodeItem(CodeItem *&codeItem) {
        code_ = codeItem;
    }


    void MethodItem::SetClassData(ClassData *&classdata) {
        classData_ = classdata;
    }

    ClassData *MethodItem::GetClassData() const {
        return classData_;
    }

    void MethodItem::SetAccessFlags(uint32_t access_flags) {
        access_flags_ = access_flags;
    }

    std::string MethodItem::GetSignature() const {
        return this->GetMethodId()->Proto()->GetSignatureForProtoId();
    }

    std::string MethodItem::GetMethodName() const {
        return this->GetMethodId()->Name()->Data();
    }

    std::string MethodItem::GetRawClassName() const {
        return this->GetClassData()->GetClassDef()->getClassName();
    }

    std::string MethodItem::GetClassPackageName() const {
        return this->GetClassData()->GetClassDef()->getPackage();
    }

    uint32_t MethodItem::GetRawId() const {
        return this->raw_method_index;
    }

    void MethodItem::AddMethodRef(uint32_t id) {
        this->method_ref_idx.insert(id);
    }

    void MethodItem::AddStringRef(uint32_t id) {
        this->string_ref_idx.insert(id);
    }

    void MethodItem::AddTypeRef(uint32_t id) {
        this->type_ref_idx.insert(id);
    }

    void MethodItem::AddFieldRef(uint32_t raw_id) {
        this->field_ref_idx.insert(raw_id);
    }

    std::string MethodItem::GetJavaClassName() const {
        return this->GetClassData()->GetClassDef()->getJavaClassName();
    }

    std::set<uint32_t> MethodItem::GetMethodRefs() const {
        return this->method_ref_idx;
    }

    std::set<uint32_t> MethodItem::GetStringRefs() const {
        return this->string_ref_idx;
    }

    std::set<uint32_t> MethodItem::GetTypeRefs() const {
        return this->type_ref_idx;
    }

    std::set<uint32_t> MethodItem::GetFieldRefs() const {
        return type_ref_idx;
    }

    MethodItem::MethodItem(uint32_t access_flags, const MethodId *method_id, CodeItem *code, uint32_t raw_index)
            : access_flags_(
            access_flags), method_id_(method_id), code_(code), raw_method_index(raw_index) {

    }

    MethodItem::~MethodItem() {
        //std::cout << this << std::endl;
    }

    CodeItem *MethodItem::GetCodeItem() {
        return this->code_;
    }

    void MethodItem::RestCodeItem() {
        this->code_ = nullptr;
    }

    bool MethodItem::isNative() {
        return (access_flags_ & 0x00000100) != 0;;
    }

    bool MethodItem::isConstructor() {
        return (access_flags_ & 0x00010000) != 0;;
    }

    bool MethodItem::isAccAbstract() {
        return (access_flags_ & 0x0400) != 0;;
    }

    bool MethodItem::isStatic() {
        return (access_flags_ & 0x0008) != 0;;
    }


}