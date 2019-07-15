//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_TYPELIST_H
#define WAAPOLICY_DEX_IR_TYPELIST_H

#include "dex_ir_item.h"
#include "dex_ir_typeid.h"
namespace dex_ir {

    class TypeList : public Item {
    public:
        explicit TypeList(TypeIdVector* type_list) : type_list_(type_list) {
            size_ = sizeof(uint32_t) + (type_list->size() * sizeof(uint16_t));
        }
        ~TypeList() override { }

        const TypeIdVector* GetTypeList() const { return type_list_.get(); }

    private:
        std::unique_ptr<TypeIdVector> type_list_;

        DISALLOW_COPY_AND_ASSIGN(TypeList);
    };

}

#endif //WAAPOLICY_DEX_IR_TYPELIST_H
