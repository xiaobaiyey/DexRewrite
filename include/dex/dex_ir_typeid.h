//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_TYPEID_H
#define WAAPOLICY_DEX_IR_TYPEID_H

#include "dex_ir_item.h"
#include "dex_ir.h"
#include <vector>
namespace dex_ir {
    class TypeId : public IndexedItem {
    public:
        explicit TypeId(StringId *string_id);

        ~TypeId() override {}

        static size_t ItemSize() { return kTypeIdItemSize; }

        StringId *GetStringId() const;

    private:
        StringId *string_id_;

        DISALLOW_COPY_AND_ASSIGN(TypeId);
    };

    using TypeIdVector = std::vector<const TypeId *>;
}


#endif //WAAPOLICY_DEX_IR_TYPEID_H
