//
// Created by xiaob on 2020/1/1.
//
#include "type_id.h"

namespace dex_ir {

    TypeId::TypeId(StringId *string_id) : string_id_(string_id) { size_ = kTypeIdItemSize; }

    TypeId::~TypeId() {

    }

    StringId *TypeId::GetStringId() const {
        return string_id_;
    }

    void TypeId::AddMethodRef(uint32_t id) {
        this->method_ref_idx.insert(id);
    }

    std::set<uint32_t> TypeId::GetMethodRefs() const {
        return this->method_ref_idx;
    }

}
