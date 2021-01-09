//
// Created by xiaobai on 2021/1/9.
//

#ifndef DEXREWRITE_BASEREPAIR_H
#define DEXREWRITE_BASEREPAIR_H

#include <memory>
#include <libdex/header.h>
#include <libdex/dex_writer.h>

class BaseRepair {
public:
    BaseRepair(uint8_t *dex_data, size_t dex_len) {
        this->dex_data = dex_data;
        this->dex_len = dex_len;
        mDexFile = std::unique_ptr<libdex::DexFile>(libdex::DexFile::getDexFile(dex_data, dex_len));
        if (mDexFile->IsMagicValid() && mDexFile->IsVersionValid()) {
            mHeader = std::unique_ptr<dex_ir::Header>(dex_ir::DexIrBuilder(*mDexFile.get(), false));
        }

    }

protected:
    std::unique_ptr<libdex::DexFile> mDexFile;
    std::unique_ptr<dex_ir::Header> mHeader;
    uint8_t *dex_data;
    size_t dex_len;


protected:
#define kNumPackedOpcodes 0x100
    typedef unsigned char InstructionWidth;
    InstructionWidth gInstructionWidthTable[kNumPackedOpcodes] = {
            // BEGIN(libdex-widths); GENERATED AUTOMATICALLY BY opcode-gen
            1, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 2, 3, 2, 2, 3, 5, 2, 2, 3, 2, 1, 1, 2,
            2, 1, 2, 2, 3, 3, 3, 1, 1, 2, 3, 3, 3, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0,
            0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3,
            3, 3, 3, 0, 3, 3, 3, 3, 3, 0, 0, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 3, 3,
            3, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 0,
            // END(libdex-widths)
    };
};


#endif //DEXREWRITE_BASEREPAIR_H
