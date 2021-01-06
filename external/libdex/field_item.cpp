//
// Created by xiaob on 2020/1/1.
//
#include "field_item.h"

namespace dex_ir {

    FieldItem::FieldItem(uint32_t access_flags, const FieldId *field_id)
            : access_flags_(access_flags), field_id_(field_id) {}

    FieldItem::~FieldItem() {

    }

    uint32_t FieldItem::GetAccessFlags() const {
        return this->access_flags_;
    }

    const FieldId *FieldItem::GetFieldId() const {
        return this->field_id_;
    }

    void FieldItem::SetClassData(ClassData *&classdata) {
        this->classData_ = classdata;
    }

    ClassData *FieldItem::GetClassData() {
        return this->classData_;
    }
}
