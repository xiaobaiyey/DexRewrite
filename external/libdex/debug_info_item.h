//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_DEBUG_INFO_ITEM_H
#define BASE_DEBUG_INFO_ITEM_H

#include "item.h"

namespace dex_ir {
    class DebugInfoItem : public Item {
    public:
        DebugInfoItem(uint32_t debug_info_size, uint8_t *debug_info)
                : debug_info_size_(debug_info_size), debug_info_(debug_info) {}

        uint32_t GetDebugInfoSize() const { return debug_info_size_; }

        uint8_t *GetDebugInfo() const { return debug_info_.get(); }

    private:
        uint32_t debug_info_size_;
        std::unique_ptr<uint8_t[]> debug_info_;

        DISALLOW_COPY_AND_ASSIGN(DebugInfoItem);
    };


}

#endif //BASE_DEBUG_INFO_ITEM_H
