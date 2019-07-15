/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:12 AM
* @ class describe
*/

#ifndef UNPACKER_CODEITEM_H
#define UNPACKER_CODEITEM_H


#include "dex_ir.h"
#include "dex_ir_item.h"
#include "dex_ir_try_item.h"
#include "dex_ir_catchhandler.h"
#include "iteration_range.h"
#include <stdint.h>
#include "dex_instruction_iterator.h"
namespace dex_ir {
    class CodeItem : public Item {
    public:
        CodeItem(uint16_t registers_size, uint16_t ins_size, uint16_t outs_size, DebugInfoItem *debug_info,
                 uint32_t insns_size, uint16_t *insns, TryItemVector *tries, CatchHandlerVector *handlers);

        ~CodeItem() override {}

        void SetMethodItem(MethodItem *&ethodItem);

        MethodItem *GetMethodItem() const;

        std::string GetMethodName() const;

        uint16_t RegistersSize() const { return registers_size_; }


        void SetRegistersSize(uint16_t size_) {
            registers_size_ = size_;
        }

        uint16_t InsSize() const { return ins_size_; }

        void SetInsSize(uint16_t size_) {
            ins_size_ = size_;
        }

        uint16_t OutsSize() const { return outs_size_; }

        void SetOutsSize(uint16_t size_) {
            outs_size_ = size_;
        }

        uint16_t TriesSize() const { return tries_ == nullptr ? 0 : tries_->size(); }

        DebugInfoItem *DebugInfo() const { return debug_info_; }

        uint32_t InsnsSize() const { return insns_size_; }

        void SetInsnsSize(uint16_t size_) {
            insns_size_ = size_;
        }

        uint16_t *Insns() const { return insns_.get(); }

        void RestInsns(uint16_t *data) {
            insns_ = std::unique_ptr<uint16_t[]>(data);
        }

        void SetDebugInfoOffset(uint32_t off) {
            this->debuf_info_off_ = off;
        }

        uint32_t GetDebugInfoOffset() {
            return debuf_info_off_;
        }

        TryItemVector *Tries() const { return tries_.get(); }

        CatchHandlerVector *Handlers() const { return handlers_.get(); }


        art::IterationRange<art::DexInstructionIterator> Instructions() const;

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
        uint32_t debuf_info_off_;

        DISALLOW_COPY_AND_ASSIGN(CodeItem);
    };

}

#endif //UNPACKER_CODEITEM_H
