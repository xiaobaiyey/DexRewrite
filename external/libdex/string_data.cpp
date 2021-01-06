//
// Created by xiaob on 2020/1/1.
//
#include <libbase/stl_util.h>
#include "string_data.h"
#include <string.h>
namespace dex_ir {

    StringData::StringData(const char *data) : data_(strdup(data)) {
        size_ = base::UnsignedLeb128Size(libdex::CountModifiedUtf8Chars(data)) + strlen(data);
    }

    const char *StringData::Data() const { return data_.get(); }

    bool StringData::SetData(const char *data) {
        data_.reset(nullptr);
        data_ = base::UniqueCPtr<const char>(strdup(data));
        return true;
    }
}