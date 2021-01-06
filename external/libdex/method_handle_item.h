//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_METHOD_HANDLE_ITEM_H
#define BASE_METHOD_HANDLE_ITEM_H

#include "indexed_item.h"
#include "dexir_util.h"
#include "dex/dex_file.h"

namespace dex_ir {
    class MethodHandleItem : public IndexedItem {
    public:
        MethodHandleItem(libdex::DexFile::MethodHandleType method_handle_type, IndexedItem *field_or_method_id)
                : method_handle_type_(method_handle_type),
                  field_or_method_id_(field_or_method_id) {
            size_ = kMethodHandleItemSize;
        }

        ~MethodHandleItem() override {}

        static size_t ItemSize() { return kMethodHandleItemSize; }

        libdex::DexFile::MethodHandleType GetMethodHandleType() const { return method_handle_type_; }

        IndexedItem *GetFieldOrMethodId() const { return field_or_method_id_; }

        //void Accept(AbstractDispatcher* dispatch) const { dispatch->Dispatch(this); }

    private:
        libdex::DexFile::MethodHandleType method_handle_type_;
        IndexedItem *field_or_method_id_;

        DISALLOW_COPY_AND_ASSIGN(MethodHandleItem);
    };
}

#endif //BASE_METHOD_HANDLE_ITEM_H
