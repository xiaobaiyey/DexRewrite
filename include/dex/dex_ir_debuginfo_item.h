/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:17 AM
* @ class describe
*/

#ifndef UNPACKER_DEBUGINFOITEM_H
#define UNPACKER_DEBUGINFOITEM_H

#include "dex_ir_item.h"
#include "dex_ir.h"
namespace dex_ir {
    class DebugInfoItem : public Item {
    public:
        DebugInfoItem(uint32_t debug_info_size, uint8_t *debug_info);

        uint32_t GetDebugInfoSize() const { return debug_info_size_; }

        uint8_t *GetDebugInfo() const { return debug_info_.get(); }

    private:
        uint32_t debug_info_size_;
        std::unique_ptr<uint8_t[]> debug_info_;
        DISALLOW_COPY_AND_ASSIGN(DebugInfoItem);
    };
}

#endif //UNPACKER_DEBUGINFOITEM_H
