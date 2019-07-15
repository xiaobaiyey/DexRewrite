/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 10:07 PM
* @ class describe
*/


#include <dex/dex_ir_fieid_item.h>

namespace dex_ir {

    FieldItem::FieldItem(uint32_t access_flags, const FieldId *field_id) : access_flags_(
            access_flags), field_id_(field_id) {}

    uint32_t FieldItem::GetAccessFlags() const {
        return access_flags_;
    }

    const FieldId *FieldItem::GetFieldId() const {
        return field_id_;
    }
}