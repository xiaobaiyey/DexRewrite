//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_CODE_ITEM_H
#define BASE_CODE_ITEM_H

#include "item.h"
#include "try_item.h"
#include "dex/code_item_accessors.h"
#include "dex/code_item_accessors-inl.h"
#include "debug_info_item.h"
#include <libbase/iteration_range.h>

namespace dex_ir {
    class MethodItem;

    class CodeItem : public Item {
    public:
        CodeItem(uint16_t registers_size,
                 uint16_t ins_size,
                 uint16_t outs_size,
                 DebugInfoItem *debug_info,
                 uint32_t insns_size,
                 uint16_t *insns,
                 TryItemVector *tries,
                 CatchHandlerVector *handlers);

        ~CodeItem() override;

        uint16_t RegistersSize() const;

        uint16_t InsSize() const;

        uint16_t OutsSize() const;

        uint16_t TriesSize() const;

        DebugInfoItem *DebugInfo() const;

        uint32_t InsnsSize() const;

        uint16_t *Insns() const;

        TryItemVector *Tries() const;

        CatchHandlerVector *Handlers() const;

        base::IterationRange<libdex::DexInstructionIterator> Instructions() const;

        void SetMethodItem(dex_ir::MethodItem *&ethodItem);

        MethodItem *GetMethodItem() const;



        void SetRegistersSize(uint16_t size_);

        void SetInsSize(uint16_t size_);

        void SetOutsSize(uint16_t size_);

        void SetInsnsSize(uint16_t size_);

        void RestInsns(uint16_t *data);

        void SetDebugInfoOffset(uint32_t off);

        uint32_t GetDebugInfoOffset() const;

    private:
        uint16_t registers_size_;
        uint16_t ins_size_;
        uint16_t outs_size_;
        DebugInfoItem *debug_info_;  // This can be nullptr.
        uint32_t insns_size_;
        std::unique_ptr<uint16_t[]> insns_;
        std::unique_ptr<TryItemVector> tries_;  // This can be nullptr.
        std::unique_ptr<CatchHandlerVector> handlers_;  // This can be nullptr.

        MethodItem *methodItem_ = nullptr;
        //add start
        uint32_t debuf_info_off_ = 0;
        DISALLOW_COPY_AND_ASSIGN(CodeItem);
    };
}


#endif //BASE_CODE_ITEM_H
