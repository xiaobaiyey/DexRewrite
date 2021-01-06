//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_METHOD_ITEM_H
#define BASE_METHOD_ITEM_H

#include "item.h"
#include "method_id.h"
#include "code_item.h"

namespace dex_ir {
    class ClassData;

    class MethodItem : public Item {
    public:
        MethodItem(uint32_t access_flags, const MethodId *method_id, CodeItem *code, uint32_t raw_index);

        ~MethodItem() override;

        MethodItem(MethodItem &&) = default;

        uint32_t GetAccessFlags() const;

        const MethodId *GetMethodId() const;

        CodeItem *GetCodeItem();

        uint32_t GetRawId() const;

        void SetCodeItem(CodeItem *&codeItem);

        void RestCodeItem();

        void SetClassData(ClassData *&classdata);

        ClassData *GetClassData() const;

        void SetAccessFlags(uint32_t access_flags);

        std::string GetSignature() const;

        std::string GetMethodName() const;

        std::string GetRawClassName() const;

        std::string GetJavaClassName() const;

        std::string GetClassPackageName() const;

        bool isNative();

        bool isConstructor();

        bool isAccAbstract();

        bool isStatic();

        void AddMethodRef(uint32_t raw_id);

        /**调用了那些方法*/
        std::set<uint32_t> GetMethodRefs() const;

        void AddStringRef(uint32_t raw_id);

        std::set<uint32_t> GetStringRefs() const;

        void AddTypeRef(uint32_t raw_id);

        std::set<uint32_t> GetTypeRefs() const;

        void AddFieldRef(uint32_t raw_id);

        std::set<uint32_t> GetFieldRefs() const;

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        uint32_t access_flags_;
        const MethodId *method_id_;
        CodeItem *code_;  // This can be nullptr.
        ClassData *classData_ = nullptr;
        uint32_t raw_method_index;
        std::set<uint32_t> method_ref_idx;
        std::set<uint32_t> string_ref_idx;
        std::set<uint32_t> type_ref_idx;
        std::set<uint32_t> field_ref_idx;
        DISALLOW_COPY_AND_ASSIGN(MethodItem);
    };

    using MethodItemVector = std::vector<std::unique_ptr<MethodItem>>;

}


#endif //BASE_METHOD_ITEM_H
