/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/18 12:56 AM
* @ class describe
*/

#ifndef UNPACKER_CODEITEMDATA_H
#define UNPACKER_CODEITEMDATA_H
// Abstracts accesses to the instruction fields of code items for CompactDexFile and
// StandardDexFile.

#include <stdint.h>
#include <stdlib.h>
#include "libdex/base/IterationRange.h"
#include "libdex/base/StandardDexFile.h"
namespace libdex {
    class DexFile;
    class DexInstructionIterator;
    class Instruction;
    class CodeItemInstructionAccessor {
    public:
        CodeItemInstructionAccessor(const DexFile &dex_file,
                                    const DexFile::CodeItem *code_item);

        //explicit CodeItemInstructionAccessor(ArtMethod* method);

        DexInstructionIterator begin() const;

        DexInstructionIterator end() const;

        IterationRange <DexInstructionIterator> InstructionsFrom(uint32_t start_dex_pc) const;

        uint32_t InsnsSizeInCodeUnits() const;

        const uint16_t *Insns() const ;

        // Return the instruction for a dex pc.
        const Instruction &InstructionAt(uint32_t dex_pc) const;

        // Return true if the accessor has a code item.
        bool HasCodeItem() const ;

    protected:
        CodeItemInstructionAccessor() = default;

        void Init(uint32_t insns_size_in_code_units, const uint16_t *insns);

        void Init(const StandardDexFile::CodeItem &code_item);

        void Init(const DexFile &dex_file, const DexFile::CodeItem *code_item);

    private:
        // size of the insns array, in 2 byte code units. 0 if there is no code item.
        uint32_t insns_size_in_code_units_ = 0;

        // Pointer to the instructions, null if there is no code item.
        const uint16_t *insns_ = 0;
    };


// Abstracts accesses to code item fields other than debug info for CompactDexFile and
// StandardDexFile.
    class CodeItemDataAccessor : public CodeItemInstructionAccessor {
    public:
        CodeItemDataAccessor(const DexFile &dex_file, const DexFile::CodeItem *code_item);

        uint16_t RegistersSize() const;

        uint16_t InsSize() const;

        uint16_t OutsSize() const;

        uint16_t TriesSize() const ;

        IterationRange<const DexFile::TryItem *> TryItems() const;

        const uint8_t *GetCatchHandlerData(size_t offset = 0) const;

        const DexFile::TryItem *FindTryItem(uint32_t try_dex_pc) const;

         const void *CodeItemDataEnd() const;

    protected:
        CodeItemDataAccessor() = default;

        void Init(const StandardDexFile::CodeItem &code_item);

        void Init(const DexFile &dex_file, const DexFile::CodeItem *code_item);

    private:
        // Fields mirrored from the cdex code item.
        uint16_t registers_size_;
        uint16_t ins_size_;
        uint16_t outs_size_;
        uint16_t tries_size_;
    };

// Abstract accesses to code item data including debug info offset. More heavy weight than the other
// helpers.
    class CodeItemDebugInfoAccessor : public CodeItemDataAccessor {
    public:
        CodeItemDebugInfoAccessor() = default;

        // Initialize with an existing offset.
        CodeItemDebugInfoAccessor(const DexFile &dex_file,
                                  const DexFile::CodeItem *code_item,
                                  uint32_t dex_method_index) {
            Init(dex_file, code_item, dex_method_index);
        }

        void Init(const DexFile &dex_file,
                  const DexFile::CodeItem *code_item,
                  uint32_t dex_method_index);

        // explicit CodeItemDebugInfoAccessor(ArtMethod* method);

        uint32_t DebugInfoOffset() const {
            return debug_info_offset_;
        }

        template<typename NewLocalCallback>
        bool DecodeDebugLocalInfo(bool is_static,
                                  uint32_t method_idx,
                                  NewLocalCallback new_local,
                                  void *context) const;

    protected:
        void Init(const StandardDexFile::CodeItem &code_item);

    private:
        const DexFile *dex_file_ = nullptr;
        uint32_t debug_info_offset_ = 0u;
    };
}
#endif //UNPACKER_CODEITEMDATA_H
