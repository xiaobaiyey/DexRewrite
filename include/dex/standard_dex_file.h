/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/17 5:50 PM
* @ class describe
*/

#ifndef UNPACKER_STANDARDDEXFILE_H
#define UNPACKER_STANDARDDEXFILE_H

#include "dex_file.h"

namespace art {
    class StandardDexFile : public DexFile {
    public:
        class Header : public DexFile::Header {
            // Same for now.
        };

        struct CodeItem : public DexFile::CodeItem {
            static constexpr size_t kAlignment = 4;

        public:
            CodeItem() = default;

            uint16_t registers_size_;            // the number of registers used by this code
            //   (locals + parameters)
            uint16_t ins_size_;                  // the number of words of incoming arguments to the method
            //   that this code is for
            uint16_t outs_size_;                 // the number of words of outgoing argument space required
            //   by this code for method invocation
            uint16_t tries_size_;                // the number of try_items for this instance. If non-zero,
            //   then these appear as the tries array just after the
            //   insns in this instance.
            uint32_t debug_info_off_;            // Holds file offset to debug info stream.

            uint32_t insns_size_in_code_units_;  // size of the insns array, in 2 byte code units
            uint16_t insns_[1];                  // actual array of bytecode.

            friend class CodeItemDataAccessor;

            friend class CodeItemDebugInfoAccessor;

            friend class CodeItemInstructionAccessor;

            friend class DexWriter;

            friend class StandardDexFile;
            DISALLOW_COPY_AND_ASSIGN(CodeItem);
        };

        //Write the standard dex specific magic.
        static void WriteMagic(uint8_t *magic);

        // Write the current version, note that the input is the address of the magic.
        static void WriteCurrentVersion(uint8_t *magic);

        static const uint8_t kDexMagic[kDexMagicSize];
        static constexpr size_t kNumDexVersions = 4;
        static const uint8_t kDexMagicVersions[kNumDexVersions][kDexVersionLen];

        // Returns true if the byte string points to the magic value.
        static bool IsMagicValid(const uint8_t *magic);

        virtual bool IsMagicValid() const override;

        // Returns true if the byte string after the magic is the correct value.
        static bool IsVersionValid(const uint8_t *magic);

        virtual bool IsVersionValid() const override;

        virtual bool SupportsDefaultMethods() const override;

        uint32_t GetCodeItemSize(const DexFile::CodeItem &item) const override;

        virtual size_t GetDequickenedSize() const override {
            return Size();
        }

    public:
        StandardDexFile(const uint8_t *base,
                        size_t size,
                        const std::string &location,
                        uint32_t location_checksum, bool is_from_file)
                : DexFile(base,
                          size,
                /*data_begin*/ base,
                /*data_size*/ size,
                          location,
                          location_checksum,
                /*is_compact_dex*/ false, is_from_file) {

        }


        DISALLOW_COPY_AND_ASSIGN(StandardDexFile);
    };


}
#endif //UNPACKER_STANDARDDEXFILE_H
