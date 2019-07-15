/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 10:09 PM
* @ class describe
*/


#include <dex/dex_ir_method_item.h>
#include <dex/dex_ir_methodid.h>
#include <dex/dex_ir_protoid.h>
#include <dex/dex_ir_stringid.h>
#include <dex/dex_ir_stringdata.h>
#include <dex/dex_ir_classdef.h>
#include <dex/dex_ir_classdata.h>
#include <dex/dex_ir_typeid.h>
namespace dex_ir {

    MethodItem::MethodItem(uint32_t access_flags, const MethodId *method_id, CodeItem *code, uint32_t raw_index)
            : access_flags_(access_flags), method_id_(method_id), code_(code), raw_method_index(raw_index) {}

    uint32_t MethodItem::GetAccessFlags() const {
        return access_flags_;
    }

    const MethodId *MethodItem::GetMethodId() const {
        return method_id_;
    }

    void MethodItem::SetCodeItem(CodeItem *&codeItem) {
        code_ = codeItem;
    }

    CodeItem *MethodItem::GetCodeItem() const {
        return code_;
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

    std::string MethodItem::GetSignure() const {
        return this->GetMethodId()->Proto()->GetSignatureForProtoId();
    }

    std::string MethodItem::GetMethodName() const {
        return this->GetMethodId()->Name()->Data();
    }

    std::string MethodItem::GetClassName() const {
        return this->GetClassData()->GetClassDef()->ClassType()->GetStringId()->Data();
    }


}