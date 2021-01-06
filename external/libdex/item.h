//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_ITEM_H
#define BASE_ITEM_H

#include <libbase/macros.h>
#include <stdint.h>
#include <libbase/logging.h>
namespace dex_ir {
    class Item {
    public:
        Item() {}

        virtual ~Item() {}

        Item(Item &&) = default;

        // Return the assigned offset.
        uint32_t GetOffset() const WARN_UNUSED {
            CHECK(OffsetAssigned());
            return offset_;
        }

        uint32_t GetSize() const WARN_UNUSED { return size_; }

        void SetOffset(uint32_t offset) { offset_ = offset; }

        void SetSize(uint32_t size) { size_ = size; }

        bool OffsetAssigned() const {
            return offset_ != kOffsetUnassigned;
        }

    protected:
        Item(uint32_t offset, uint32_t size) : offset_(offset), size_(size) {}

        // 0 is the dex file header and shouldn't be a valid offset for any part of the dex file.
        static constexpr uint32_t kOffsetUnassigned = 0u;

        // Start out unassigned.
        uint32_t offset_ = kOffsetUnassigned;
        uint32_t size_ = 0;
    };

}


#endif //BASE_ITEM_H
