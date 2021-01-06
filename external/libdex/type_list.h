//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_TYPE_LIST_H
#define BASE_TYPE_LIST_H

#include "item.h"
#include "type_id.h"

namespace dex_ir {
    class TypeList : public Item {
    public:
        explicit TypeList(TypeIdVector *type_list) : type_list_(type_list) {
            size_ = sizeof(uint32_t) + (type_list->size() * sizeof(uint16_t));
        }

        ~TypeList() override {}

        const TypeIdVector *GetTypeList() const { return type_list_.get(); }

    private:
        std::unique_ptr<TypeIdVector> type_list_;

        DISALLOW_COPY_AND_ASSIGN(TypeList);
    };
}

#endif //BASE_TYPE_LIST_H
