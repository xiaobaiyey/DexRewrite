//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_COLLECTION_BASE_H
#define BASE_COLLECTION_BASE_H

#include <stdint.h>
#include "libbase/macros.h"
namespace dex_ir{
    class CollectionBase {
    public:
        CollectionBase() = default;

        virtual ~CollectionBase() {}

        uint32_t GetOffset() const { return offset_; }

        void SetOffset(uint32_t new_offset) { offset_ = new_offset; }

        virtual uint32_t Size() const = 0;

        bool Empty() const { return Size() == 0u; }

    private:
        // Start out unassigned.
        uint32_t offset_ = 0u;
        DISALLOW_COPY_AND_ASSIGN(CollectionBase);
    };
}



#endif //BASE_COLLECTION_BASE_H
