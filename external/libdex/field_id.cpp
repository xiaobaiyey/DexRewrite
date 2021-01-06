//
// Created by xiaob on 2020/1/1.
//

#include "field_id.h"

namespace dex_ir {

    FieldId::FieldId(const TypeId *klass, const TypeId *type, const StringId *name)
            : class_(klass), type_(type), name_(name) { size_ = kFieldIdItemSize; }

    FieldId::~FieldId() {

    }

    const TypeId *FieldId::Class() const {
        return this->class_;
    }

    const TypeId *FieldId::Type() const {
        return this->type_;
    }

    const StringId *FieldId::Name() const {
        return this->name_;
    }

    void FieldId::AddMethodRefs(uint32_t id) {
        this->method_ref_idx.insert(id);
    }

    std::set<uint32_t> FieldId::GetMethodRefs() const {
        return this->method_ref_idx;
    }
}