//
// Created by xiaob on 2020/1/2.
//
#include "code_item.h"
#include "method_item.h"

namespace dex_ir {

    CodeItem::CodeItem(uint16_t registers_size, uint16_t ins_size, uint16_t outs_size, DebugInfoItem *debug_info,
                       uint32_t insns_size, uint16_t *insns, TryItemVector *tries, CatchHandlerVector *handlers)
            : registers_size_(registers_size),
              ins_size_(ins_size),
              outs_size_(outs_size),
              debug_info_(debug_info),
              insns_size_(insns_size),
              insns_(insns),
              tries_(tries),
              handlers_(handlers) {}

    CodeItem::~CodeItem() {

    }

    uint16_t CodeItem::RegistersSize() const {
        return this->registers_size_;
    }

    uint16_t CodeItem::InsSize() const {
        return this->ins_size_;
    }

    uint16_t CodeItem::OutsSize() const {
        return this->outs_size_;
    }

    uint16_t CodeItem::TriesSize() const {
        return tries_ == nullptr ? 0 : tries_->size();
    }

    DebugInfoItem *CodeItem::DebugInfo() const { return debug_info_; }

    uint32_t CodeItem::InsnsSize() const { return insns_size_; }

    uint16_t *CodeItem::Insns() const { return insns_.get(); }

    TryItemVector *CodeItem::Tries() const { return tries_.get(); }

    CatchHandlerVector *CodeItem::Handlers() const { return handlers_.get(); }

    base::IterationRange<libdex::DexInstructionIterator> CodeItem::Instructions() const {
        return base::MakeIterationRange(libdex::DexInstructionIterator(Insns(), 0u),
                                        libdex::DexInstructionIterator(Insns(), InsnsSize()));
    }

    void CodeItem::SetMethodItem(dex_ir::MethodItem *&ethodItem) {
        this->methodItem_ = ethodItem;
    }

    MethodItem *CodeItem::GetMethodItem() const {
        return this->methodItem_;
    }

    void CodeItem::SetRegistersSize(uint16_t size_) {
        registers_size_ = size_;
    }

    void CodeItem::SetInsSize(uint16_t size_) {
        ins_size_ = size_;
    }

    void CodeItem::SetOutsSize(uint16_t size_) {
        outs_size_ = size_;
    }

    void CodeItem::SetInsnsSize(uint16_t size_) {
        this->insns_size_ = size_;
    }

    void CodeItem::RestInsns(uint16_t *data) {
        insns_ = std::unique_ptr<uint16_t[]>(data);

    }

    void CodeItem::SetDebugInfoOffset(uint32_t off) {
        this->debuf_info_off_ = off;
    }

    uint32_t CodeItem::GetDebugInfoOffset() const {
        return debuf_info_off_;
    }




}
