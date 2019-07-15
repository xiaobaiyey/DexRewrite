//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_TYPEADDR_PAIR_H
#define WAAPOLICY_DEX_IR_TYPEADDR_PAIR_H

#include "dex_ir.h"
#include <vector>
#include <base/macros.h>


namespace dex_ir {
    class TypeAddrPair {
    public:
        TypeAddrPair(const TypeId *type_id, uint32_t address) : type_id_(type_id),
                                                                address_(address) {}

        const TypeId *GetTypeId() const { return type_id_; }

        uint32_t GetAddress() const { return address_; }

    private:
        const TypeId *type_id_;  // This can be nullptr.
        uint32_t address_;

        DISALLOW_COPY_AND_ASSIGN(TypeAddrPair);
    };

    using TypeAddrPairVector = std::vector<std::unique_ptr<const TypeAddrPair>>;
}

#endif //WAAPOLICY_DEX_IR_TYPEADDR_PAIR_H
