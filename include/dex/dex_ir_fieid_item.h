/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 10:07 PM
* @ class describe
*/

#ifndef UNPACKER_FIELDITEM_H
#define UNPACKER_FIELDITEM_H

#include "dex_ir.h"
#include "dex_ir_item.h"
namespace dex_ir {
    class FieldItem : public Item {
    public:
        FieldItem(uint32_t access_flags, const FieldId *field_id);

        ~FieldItem() override {}

        uint32_t GetAccessFlags() const;

        const FieldId *GetFieldId() const;

        void SetClassData(ClassData * &classdata) {
            classData_ = classdata;
        }

        ClassData *GetClassData() {
            return classData_;
        }

    private:
        uint32_t access_flags_;
        const FieldId *field_id_;
        ClassData *classData_;
        DISALLOW_COPY_AND_ASSIGN(FieldItem);
    };
    using FieldItemVector = std::vector<std::unique_ptr<FieldItem>>;

}


#endif //UNPACKER_FIELDITEM_H
