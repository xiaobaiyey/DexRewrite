/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_LIBDEXFILE_DEX_STANDARD_DEX_FILE_H_
#define ART_LIBDEXFILE_DEX_STANDARD_DEX_FILE_H_

#include <iosfwd>

#include "dex_file.h"

namespace dex_ir {
    class DexWriter;
}

namespace libdex {

    class OatDexFile;

// Standard dex file. This is the format that is packaged in APKs and produced by tools.
    class StandardDexFile : public DexFile {
    public:
        class Header : public DexFile::Header {
            // Same for now.
        };

        struct CodeItem : public dex::CodeItem {
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

            ART_FRIEND_TEST(CodeItemAccessorsTest, TestDexInstructionsAccessor);

            friend class CodeItemDataAccessor;

            friend class CodeItemDebugInfoAccessor;

            friend class CodeItemInstructionAccessor;

            friend class ::dex_ir::DexWriter;

            friend class StandardDexFile;
            DISALLOW_COPY_AND_ASSIGN(CodeItem);
        };

        // Write the standard dex specific magic.
        static void WriteMagic(uint8_t *magic);

        // Write the current version, note that the input is the address of the magic.
        static void WriteCurrentVersion(uint8_t *magic);

        static const uint8_t kDexMagic[kDexMagicSize];
        static constexpr size_t kNumDexVersions = 4;
        static const uint8_t kDexMagicVersions[kNumDexVersions][kDexVersionLen];

        // Returns true if the byte string points to the magic value.
        static bool IsMagicValid(const uint8_t *magic);

        bool IsMagicValid() const override;

        // Returns true if the byte string after the magic is the correct value.
        static bool IsVersionValid(const uint8_t *magic);

        bool IsVersionValid() const override;

        bool SupportsDefaultMethods() const override;

        uint32_t GetCodeItemSize(const dex::CodeItem &item) const override;


    public:
        StandardDexFile(const uint8_t *base,
                        size_t size,
                        const std::string &location,
                        uint32_t location_checksum,
                        std::unique_ptr<DexFileContainer> container)
                : DexFile(base,
                          size,
                /*data_begin*/ base,
                /*data_size*/ size,
                          location,
                          location_checksum,
                          std::move(container),
                /*is_compact_dex*/ false) {}

        size_t GetDequickenedSize() const override;

        friend class DexFileLoader;

        friend class DexFileVerifierTest;

        ART_FRIEND_TEST(ClassLinkerTest, RegisterDexFileName);  // for constructor
        friend class OptimizingUnitTestHelper;  // for constructor

        DISALLOW_COPY_AND_ASSIGN(StandardDexFile);
    };

}  // namespace libdex

#endif  // ART_LIBDEXFILE_DEX_STANDARD_DEX_FILE_H_
