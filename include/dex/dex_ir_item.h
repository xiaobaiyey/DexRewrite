//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_ITEM_H
#define WAAPOLICY_DEX_IR_ITEM_H

#include <stdint.h>
#include <base/logging.h>

namespace dex_ir {
    class Item {
    public:
        Item() {}

        virtual ~Item() {}

        // Return the assigned offset.
        uint32_t GetOffset() const {
            CHECK(OffsetAssigned());
            return offset_;
        }

        uint32_t GetSize() const { return size_; }

        void SetOffset(uint32_t offset) { offset_ = offset; }

        void SetSize(uint32_t size) { size_ = size; }

        bool OffsetAssigned() const {
            //LOGI("offset_:%d kOffsetUnassigned:%d", offset_, kOffsetUnassigned);
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

    class IndexedItem : public Item {
    public:
        IndexedItem() {}

        virtual ~IndexedItem() {}

        uint32_t GetIndex() const { return index_; }

        void SetIndex(uint32_t index) { index_ = index; }

    protected:
        IndexedItem(uint32_t offset, uint32_t size, uint32_t index)
                : Item(offset, size), index_(index) {}

        uint32_t index_ = 0;
    };
}


#endif //WAAPOLICY_DEX_IR_ITEM_H
