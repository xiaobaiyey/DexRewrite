//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_FIELD_ITEM_H
#define BASE_FIELD_ITEM_H

#include "item.h"
#include "field_id.h"

namespace dex_ir {
    class ClassData;

    class FieldItem : public Item {
    public:
        FieldItem(uint32_t access_flags, const FieldId *field_id);

        ~FieldItem() override;

        FieldItem(FieldItem &&) = default;

        uint32_t GetAccessFlags() const;

        const FieldId *GetFieldId() const;

        void SetClassData(ClassData *&classdata);

        ClassData *GetClassData();
        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        uint32_t access_flags_;
        const FieldId *field_id_;
        ClassData *classData_;
        DISALLOW_COPY_AND_ASSIGN(FieldItem);
    };

    using FieldItemVector = std::vector<FieldItem>;
}

#endif //BASE_FIELD_ITEM_H
