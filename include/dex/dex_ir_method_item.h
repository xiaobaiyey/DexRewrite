/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 10:09 PM
* @ class describe
*/

#ifndef UNPACKER_METHODITEM_H
#define UNPACKER_METHODITEM_H


#include "dex_ir.h"
#include "dex_ir_item.h"
namespace dex_ir {

    class MethodItem : public Item {
    public:
        MethodItem(uint32_t access_flags, const MethodId *method_id, CodeItem *code,uint32_t raw_index);

        ~MethodItem() override {}

        uint32_t GetAccessFlags() const;

        const MethodId *GetMethodId() const;

        CodeItem *GetCodeItem() const;

        void SetCodeItem(CodeItem * &codeItem);

        void SetClassData(ClassData * &classdata);

        ClassData *GetClassData() const;

        void SetAccessFlags(uint32_t access_flags);

        std::string GetSignure()const ;

        std::string GetMethodName() const;

        std::string GetClassName()const;



    private:
        uint32_t access_flags_;
        const MethodId *method_id_;
        CodeItem *code_;  // This can be nullptr.
        ClassData *classData_ = nullptr;
        uint32_t raw_method_index;

        DISALLOW_COPY_AND_ASSIGN(MethodItem);
    };

    using MethodItemVector = std::vector<std::unique_ptr<MethodItem>>;
}


#endif //UNPACKER_METHODITEM_H
