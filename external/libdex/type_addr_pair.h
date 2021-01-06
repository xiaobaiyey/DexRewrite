//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_TYPE_ADDR_PAIR_H
#define BASE_TYPE_ADDR_PAIR_H

#include <cstdint>
#include "type_id.h"

namespace dex_ir{
    class TypeAddrPair {
    public:
        TypeAddrPair(const TypeId* type_id, uint32_t address) : type_id_(type_id), address_(address) { }

        const TypeId* GetTypeId() const { return type_id_; }
        uint32_t GetAddress() const { return address_; }

    private:
        const TypeId* type_id_;  // This can be nullptr.
        uint32_t address_;

        DISALLOW_COPY_AND_ASSIGN(TypeAddrPair);
    };

    using TypeAddrPairVector = std::vector<std::unique_ptr<const TypeAddrPair>>;
}

#endif //BASE_TYPE_ADDR_PAIR_H
