/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:08 AM
* @ class describe
*/

#ifndef UNPACKER_TRYITEM_H
#define UNPACKER_TRYITEM_H

#include "dex_ir.h"
#include "dex_ir_item.h"
namespace dex_ir {
    class TryItem : public Item {
    public:
        TryItem(uint32_t start_addr, uint16_t insn_count, const CatchHandler *handlers);

        ~TryItem() override {}

        uint32_t StartAddr() const { return start_addr_; }

        uint16_t InsnCount() const { return insn_count_; }

        const CatchHandler *GetHandlers() const { return handlers_; }


    private:
        uint32_t start_addr_;
        uint16_t insn_count_;
        const CatchHandler *handlers_;

        DISALLOW_COPY_AND_ASSIGN(TryItem);
    };

    using TryItemVector = std::vector<std::unique_ptr<const TryItem>>;
}


#endif //UNPACKER_TRYITEM_H
