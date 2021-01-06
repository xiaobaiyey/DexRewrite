//
// Created by xiaob on 2020/1/1.
//

#include "string_id.h"

namespace dex_ir {

    StringId::StringId(StringData *string_data) : string_data_(string_data) {
        size_ = kStringIdItemSize;
    }

    StringId::~StringId() {

    }

    size_t StringId::ItemSize() { return kStringIdItemSize; }

    const char *StringId::Data() const { return string_data_->Data(); }

    StringData *StringId::DataItem() const { return string_data_; }

    void StringId::AddMethodRefs(uint32_t id) {
        this->method_ref_idx.insert(id);
    }

    std::set<uint32_t> StringId::GetMethodRefs() const {
        return this->method_ref_idx;
    }
}