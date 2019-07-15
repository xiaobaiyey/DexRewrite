//
// Created by xiaobaiyey on 2019-07-15.
//

#include <dex/dex_ir_stringid.h>
#include <dex/dex_ir_stringdata.h>
namespace dex_ir {

    StringId::StringId(StringData *string_data) : string_data_(string_data) {
        size_ = kStringIdItemSize;
    }

    const char *StringId::Data() const {
        return string_data_->Data();
    }

    StringData *StringId::DataItem() const {
        return string_data_;
    }
}