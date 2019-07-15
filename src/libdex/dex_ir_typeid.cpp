//
// Created by xiaobaiyey on 2019-07-15.
//

#include <dex/dex_ir_typeid.h>
#include <dex/dex_ir.h>

namespace dex_ir {

    TypeId::TypeId(StringId *string_id) : string_id_(
            string_id) { size_ = kTypeIdItemSize; }

    StringId *TypeId::GetStringId() const {
        return string_id_;
    }

}

