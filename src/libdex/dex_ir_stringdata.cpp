//
// Created by xiaobaiyey on 2019-07-15.
//

#include "dex/dex_ir_stringdata.h"
#include "dex_utils.h"
#include "dex/leb128.h"

namespace dex_ir {
    StringData::StringData(const char *data) : data_(strdup(data)) {
        size_ = UnsignedLeb128Size(CountModifiedUtf8Chars(data)) + strlen(data);
    }

    const char *StringData::Data() const {
        return data_.get();
    }

    bool StringData::SetData(const char *data) {
        data_.reset(nullptr);
        data_ = UniqueCPtr<const char>(strdup(data));
        return true;
    }
}
