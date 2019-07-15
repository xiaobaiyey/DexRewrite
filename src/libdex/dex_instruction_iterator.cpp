//
// Created by xiaobaiyey on 2019-07-15.
//

#include <dex/dex_instruction_iterator.h>


namespace art {
    const Instruction &DexInstructionPcPair::Inst() const {
        return *Instruction::At(instructions_ + DexPc());
    }

    const Instruction *DexInstructionPcPair::operator->() const {
        return &Inst();
    }

    uint32_t DexInstructionPcPair::DexPc() const {
        return dex_pc_;
    }

    const uint16_t *DexInstructionPcPair::Instructions() const {
        return instructions_;
    }

    const Instruction &DexInstructionIteratorBase::Inst() const {
        return data_.Inst();
    }

    uint32_t DexInstructionIteratorBase::DexPc() const {
        return data_.DexPc();
    }

    const uint16_t *DexInstructionIteratorBase::Instructions() const {
        return data_.Instructions();
    }

    DexInstructionIterator &DexInstructionIterator::operator++() {
        data_.dex_pc_ += Inst().SizeInCodeUnits();
        return *this;
    }

    DexInstructionIterator DexInstructionIterator::operator++(int) {
        DexInstructionIterator temp = *this;
        ++*this;
        return temp;
    }

    const DexInstructionPcPair &DexInstructionIterator::operator*() const {
        return data_;
    }

    const Instruction *DexInstructionIterator::operator->() const {
        return &data_.Inst();
    }

    uint32_t DexInstructionIterator::DexPc() const {
        return data_.DexPc();
    }

    SafeDexInstructionIterator &SafeDexInstructionIterator::operator++() {
        AssertValid();
        const size_t size_code_units = Inst().CodeUnitsRequiredForSizeComputation();
        const size_t available = NumCodeUnits() - DexPc();
        if (UNLIKELY(size_code_units > available)) {
            error_state_ = true;
            return *this;
        }
        const size_t instruction_code_units = Inst().SizeInCodeUnits();
        if (UNLIKELY(instruction_code_units > available)) {
            error_state_ = true;
            return *this;
        }
        data_.dex_pc_ += instruction_code_units;
        return *this;
    }

    SafeDexInstructionIterator SafeDexInstructionIterator::operator++(int) {
        SafeDexInstructionIterator temp = *this;
        ++*this;
        return temp;
    }

    const DexInstructionPcPair &SafeDexInstructionIterator::operator*() const {
        AssertValid();
        return data_;
    }

    const Instruction *SafeDexInstructionIterator::operator->() const {
        AssertValid();
        return &data_.Inst();
    }

    const Instruction &SafeDexInstructionIterator::Inst() const {
        return data_.Inst();
    }

    const uint16_t *SafeDexInstructionIterator::Instructions() const {
        return data_.Instructions();
    }

    void SafeDexInstructionIterator::AssertValid() const {
        DCHECK(!IsErrorState());
        DCHECK_LT(DexPc(), NumCodeUnits());
    }

    uint32_t SafeDexInstructionIterator::NumCodeUnits() const {
        return num_code_units_;
    }

    bool SafeDexInstructionIterator::IsErrorState() const {
        return error_state_;
    }
}