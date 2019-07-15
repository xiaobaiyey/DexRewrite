//
// Created by xiaobaiyey on 2019-07-15.
//

#include "code_item_accessors.h"
#include "dex/leb128.h"
#include <dex/dex_instruction_iterator.h>

namespace art {
    void CodeItemInstructionAccessor::Init(uint32_t insns_size_in_code_units,
                                           const uint16_t *insns) {
        insns_size_in_code_units_ = insns_size_in_code_units;
        insns_ = insns;
    }


    void CodeItemInstructionAccessor::Init(const StandardDexFile::CodeItem &code_item) {
        Init(code_item.insns_size_in_code_units_, code_item.insns_);
    }

    void CodeItemInstructionAccessor::Init(const DexFile &dex_file,
                                           const DexFile::CodeItem *code_item) {
        if (code_item != nullptr) {
            DCHECK(dex_file.IsInDataSection(code_item));
            DCHECK(dex_file.IsStandardDexFile());
            Init(down_cast<const StandardDexFile::CodeItem &>(*code_item));
        }

    }

    CodeItemInstructionAccessor::CodeItemInstructionAccessor(
            const DexFile &dex_file,
            const DexFile::CodeItem *code_item) {
        Init(dex_file, code_item);
    }

    DexInstructionIterator CodeItemInstructionAccessor::begin() const {
        return DexInstructionIterator(insns_, 0u);
    }

    DexInstructionIterator CodeItemInstructionAccessor::end() const {
        return DexInstructionIterator(insns_, insns_size_in_code_units_);
    }

    IterationRange<DexInstructionIterator> CodeItemInstructionAccessor::InstructionsFrom(
            uint32_t start_dex_pc) const {
        DCHECK_LT(start_dex_pc, InsnsSizeInCodeUnits());
        return {
                DexInstructionIterator(insns_, start_dex_pc),
                DexInstructionIterator(insns_, insns_size_in_code_units_)};
    }


    void CodeItemDataAccessor::Init(const StandardDexFile::CodeItem &code_item) {
        CodeItemInstructionAccessor::Init(code_item);
        registers_size_ = code_item.registers_size_;
        ins_size_ = code_item.ins_size_;
        outs_size_ = code_item.outs_size_;
        tries_size_ = code_item.tries_size_;
    }

    void CodeItemDataAccessor::Init(const DexFile &dex_file,
                                    const DexFile::CodeItem *code_item) {
        DCHECK(dex_file.IsStandardDexFile());
        CodeItemDataAccessor::Init(down_cast<const StandardDexFile::CodeItem &>(*code_item));
    }

    CodeItemDataAccessor::CodeItemDataAccessor(const DexFile &dex_file,
                                               const DexFile::CodeItem *code_item) {
        Init(dex_file, code_item);
    }

    IterationRange<const DexFile::TryItem *> CodeItemDataAccessor::TryItems() const {
        const DexFile::TryItem *try_items = DexFile::GetTryItems(end(), 0u);
        return {
                try_items,
                try_items + TriesSize()};
    }

    const uint8_t *CodeItemDataAccessor::GetCatchHandlerData(size_t offset) const {
        return DexFile::GetCatchHandlerData(end(), TriesSize(), offset);
    }

    const DexFile::TryItem *CodeItemDataAccessor::FindTryItem(uint32_t try_dex_pc) const {
        IterationRange<const DexFile::TryItem *> try_items(TryItems());
        int32_t index = DexFile::FindTryItem(try_items.begin(),
                                             try_items.end() - try_items.begin(),
                                             try_dex_pc);
        return index != -1 ? &try_items.begin()[index] : nullptr;
    }

    const void *CodeItemDataAccessor::CodeItemDataEnd() const {
        const uint8_t *handler_data = GetCatchHandlerData();

        if (TriesSize() == 0 || handler_data == nullptr) {
            return &end().Inst();
        }
        // Get the start of the handler data.
        const uint32_t handlers_size = DecodeUnsignedLeb128(&handler_data);
        // Manually read each handler.
        for (uint32_t i = 0; i < handlers_size; ++i) {
            int32_t uleb128_count = DecodeSignedLeb128(&handler_data) * 2;
            if (uleb128_count <= 0) {
                uleb128_count = -uleb128_count + 1;
            }
            for (int32_t j = 0; j < uleb128_count; ++j) {
                DecodeUnsignedLeb128(&handler_data);
            }
        }
        return reinterpret_cast<const void *>(handler_data);
    }

    uint16_t CodeItemDataAccessor::RegistersSize() const {
        return registers_size_;
    }

    uint16_t CodeItemDataAccessor::InsSize() const {
        return ins_size_;
    }

    uint16_t CodeItemDataAccessor::OutsSize() const {
        return outs_size_;
    }

    uint16_t CodeItemDataAccessor::TriesSize() const {
        return tries_size_;
    }

    void CodeItemDebugInfoAccessor::Init(const DexFile &dex_file,
                                         const DexFile::CodeItem *code_item,
                                         uint32_t dex_method_index) {
        if (code_item == nullptr) {
            return;
        }
        dex_file_ = &dex_file;

        DCHECK(dex_file.IsStandardDexFile());
        Init(down_cast<const StandardDexFile::CodeItem &>(*code_item));

    }


    void CodeItemDebugInfoAccessor::Init(const StandardDexFile::CodeItem &code_item) {
        debug_info_offset_ = code_item.debug_info_off_;
        CodeItemDataAccessor::Init(code_item);
    }

    const Instruction &art::CodeItemInstructionAccessor::InstructionAt(uint32_t dex_pc) const {
        DCHECK_LT(dex_pc, InsnsSizeInCodeUnits());
        return *Instruction::At(insns_ + dex_pc);
    }

    uint32_t CodeItemInstructionAccessor::InsnsSizeInCodeUnits() const {
        return insns_size_in_code_units_;
    }

    const uint16_t *CodeItemInstructionAccessor::Insns() const {
        return insns_;
    }

    bool CodeItemInstructionAccessor::HasCodeItem() const {
        return Insns() != nullptr;
    }
}