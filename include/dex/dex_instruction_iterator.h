//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_INSTRUCTION_ITERATOR_H
#define WAAPOLICY_DEX_INSTRUCTION_ITERATOR_H

#include "dex_instruction.h"
namespace art {
    class DexInstructionPcPair {
    public:
        const Instruction &Inst() const;

        const Instruction *operator->() const;

        uint32_t DexPc() const;

        const uint16_t *Instructions() const;

    protected:
        explicit DexInstructionPcPair(const uint16_t *instructions, uint32_t dex_pc)
                : instructions_(instructions), dex_pc_(dex_pc) {}

        const uint16_t *instructions_ = nullptr;
        uint32_t dex_pc_ = 0;

        friend class DexInstructionIteratorBase;

        friend class DexInstructionIterator;

        friend class SafeDexInstructionIterator;
    };

// Base helper class to prevent duplicated comparators.
    class DexInstructionIteratorBase : public std::iterator<std::forward_iterator_tag, DexInstructionPcPair> {
    public:
        using value_type = std::iterator<std::forward_iterator_tag, DexInstructionPcPair>::value_type;
        using difference_type = std::iterator<std::forward_iterator_tag, value_type>::difference_type;

        DexInstructionIteratorBase() = default;

        explicit DexInstructionIteratorBase(const Instruction *inst, uint32_t dex_pc)
                : data_(reinterpret_cast<const uint16_t *>(inst), dex_pc) {}

        const Instruction &Inst() const;

        // Return the dex pc for an iterator compared to the code item begin.
        uint32_t DexPc() const;

        // Instructions from the start of the code item.
        const uint16_t *Instructions() const;

    protected:
        DexInstructionPcPair data_;
    };

    static inline bool operator==(const DexInstructionIteratorBase &lhs,
                                  const DexInstructionIteratorBase &rhs) {
        DCHECK_EQ(lhs.Instructions(), rhs.Instructions()) << "Comparing different code items.";
        return lhs.DexPc() == rhs.DexPc();
    }

    static inline bool operator!=(const DexInstructionIteratorBase &lhs,
                                  const DexInstructionIteratorBase &rhs) {
        return !(lhs == rhs);
    }

    static inline bool operator<(const DexInstructionIteratorBase &lhs,
                                 const DexInstructionIteratorBase &rhs) {
        DCHECK_EQ(lhs.Instructions(), rhs.Instructions()) << "Comparing different code items.";
        return lhs.DexPc() < rhs.DexPc();
    }

    static inline bool operator>(const DexInstructionIteratorBase &lhs,
                                 const DexInstructionIteratorBase &rhs) {
        return rhs < lhs;
    }

    static inline bool operator<=(const DexInstructionIteratorBase &lhs,
                                  const DexInstructionIteratorBase &rhs) {
        return !(rhs < lhs);
    }

    static inline bool operator>=(const DexInstructionIteratorBase &lhs,
                                  const DexInstructionIteratorBase &rhs) {
        return !(lhs < rhs);
    }

// A helper class for a code_item's instructions using range based for loop syntax.
    class DexInstructionIterator : public DexInstructionIteratorBase {
    public:
        using DexInstructionIteratorBase::DexInstructionIteratorBase;

        explicit DexInstructionIterator(const uint16_t *inst, uint32_t dex_pc)
                : DexInstructionIteratorBase(Instruction::At(inst), dex_pc) {}

        explicit DexInstructionIterator(const DexInstructionPcPair &pair)
                : DexInstructionIterator(pair.Instructions(), pair.DexPc()) {}

        // Value after modification.
        DexInstructionIterator &operator++();

        // Value before modification.
        DexInstructionIterator operator++(int);

        const value_type &operator*() const;

        const Instruction *operator->() const;

        // Return the dex pc for the iterator.
        uint32_t DexPc() const;
    };

// A safe version of DexInstructionIterator that is guaranteed to not go past the end of the code
// item.
    class SafeDexInstructionIterator : public DexInstructionIteratorBase {
    public:
        explicit SafeDexInstructionIterator(const DexInstructionIteratorBase &start,
                                            const DexInstructionIteratorBase &end)
                : DexInstructionIteratorBase(&start.Inst(), start.DexPc()), num_code_units_(end.DexPc()) {
            DCHECK_EQ(start.Instructions(), end.Instructions())
                    << "start and end must be in the same code item.";
        }

        // Value after modification, does not read past the end of the allowed region. May increment past
        // the end of the code item though.
        SafeDexInstructionIterator &operator++();

        // Value before modification.
        SafeDexInstructionIterator operator++(int);

        const value_type &operator*() const;

        const Instruction *operator->() const;

        // Return the current instruction of the iterator.
        const Instruction &Inst() const;

        const uint16_t *Instructions() const;

        // Returns true if the iterator is in an error state. This occurs when an instruction couldn't
        // have its size computed without reading past the end iterator.
        bool IsErrorState() const;

    private:
        void AssertValid() const;

        uint32_t NumCodeUnits() const;

        const uint32_t num_code_units_ = 0;
        bool error_state_ = false;
    };
}

#endif //WAAPOLICY_DEX_INSTRUCTION_ITERATOR_H
