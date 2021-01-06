//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_INDEXED_ITEM_H
#define BASE_INDEXED_ITEM_H

#include "item.h"

namespace dex_ir {
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


#endif //BASE_INDEXED_ITEM_H
