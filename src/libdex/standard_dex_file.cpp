//
// Created by xiaobaiyey on 2019-07-15.
//

#include <dex/standard_dex_file.h>
#include "code_item_accessors.h"


namespace art {
    const uint8_t StandardDexFile::kDexMagic[] = {'d', 'e', 'x', '\n'};
    const uint8_t StandardDexFile::kDexMagicVersions[StandardDexFile::kNumDexVersions]
    [StandardDexFile::kDexVersionLen] = {
                    {'0', '3', '5', '\0'},
                    // Dex version 036 skipped because of an old dalvik bug on some versions of android where dex
                    // files with that version number would erroneously be accepted and run.
                    {'0', '3', '7', '\0'},
                    // Dex version 038: Android "O" and beyond.
                    {'0', '3', '8', '\0'},
                    // Dex verion 039: Beyond Android "O".
                    {'0', '3', '9', '\0'},
            };

    void StandardDexFile::WriteMagic(uint8_t *magic) {
        std::copy_n(kDexMagic, kDexMagicSize, magic);
    }

    void StandardDexFile::WriteCurrentVersion(uint8_t *magic) {
        std::copy_n(kDexMagicVersions[StandardDexFile::kDexVersionLen - 1],
                    kDexVersionLen,
                    magic + kDexMagicSize);
    }

    bool StandardDexFile::IsMagicValid(const uint8_t *magic) {
        return (memcmp(magic, kDexMagic, sizeof(kDexMagic)) == 0);
    }

    bool StandardDexFile::IsVersionValid(const uint8_t *magic) {
        const uint8_t *version = &magic[sizeof(kDexMagic)];
        for (uint32_t i = 0; i < kNumDexVersions; i++) {
            if (memcmp(version, kDexMagicVersions[i], kDexVersionLen) == 0) {
                return true;
            }
        }
        return false;
    }

    bool StandardDexFile::IsMagicValid() const {
        return IsMagicValid(header_->magic_);
    }

    bool StandardDexFile::IsVersionValid() const {
        return IsVersionValid(header_->magic_);
    }

    bool StandardDexFile::SupportsDefaultMethods() const {
        return GetDexVersion() >= DexFile::kDefaultMethodsVersion;
    }

    uint32_t StandardDexFile::GetCodeItemSize(const DexFile::CodeItem &item) const {
        //DCHECK(IsInDataSection(&item));
        return reinterpret_cast<uintptr_t>(CodeItemDataAccessor(*this, &item).CodeItemDataEnd()) -
               reinterpret_cast<uintptr_t>(&item);
    }
}