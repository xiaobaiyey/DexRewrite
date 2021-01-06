//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_METHOD_ID_H
#define BASE_METHOD_ID_H

#include "indexed_item.h"
#include "type_id.h"
#include "proto_id.h"

namespace dex_ir {
    class MethodId : public IndexedItem {
    public:
        MethodId(const TypeId *klass, const ProtoId *proto, const StringId *name)
                : class_(klass), proto_(proto), name_(name) { size_ = kMethodIdItemSize; }

        ~MethodId() override {}

        static size_t ItemSize() { return kMethodIdItemSize; }

        const TypeId *Class() const { return class_; }

        const ProtoId *Proto() const { return proto_; }

        const StringId *Name() const { return name_; }

        //void Accept(AbstractDispatcher *dispatch) const { dispatch->Dispatch(this); }
        void AddMethodRefs(uint32_t id) {
            this->method_ref_idx.insert(id);
        }

        std::set<uint32_t> GetMethodRefs() const {
            return this->method_ref_idx;
        }

    private:
        const TypeId *class_;
        const ProtoId *proto_;
        const StringId *name_;
        //value=method index id
        std::set<uint32_t> method_ref_idx;
        DISALLOW_COPY_AND_ASSIGN(MethodId);
    };
}


#endif //BASE_METHOD_ID_H
