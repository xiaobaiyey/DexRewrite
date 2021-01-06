//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_TRY_ITEM_H
#define BASE_TRY_ITEM_H

#include "item.h"
#include "catch_handler.h"

namespace dex_ir {
    class TryItem : public Item {
    public:
        TryItem(uint32_t start_addr, uint16_t insn_count, const CatchHandler *handlers)
                : start_addr_(start_addr), insn_count_(insn_count), handlers_(handlers) {}

        ~TryItem() override {}

        uint32_t StartAddr() const { return start_addr_; }

        uint16_t InsnCount() const { return insn_count_; }

        const CatchHandler *GetHandlers() const { return handlers_; }

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        uint32_t start_addr_;
        uint16_t insn_count_;
        const CatchHandler *handlers_;
        DISALLOW_COPY_AND_ASSIGN(TryItem);
    };

    using TryItemVector = std::vector<std::unique_ptr<const TryItem>>;

}

#endif //BASE_TRY_ITEM_H
