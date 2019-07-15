/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 1:26 AM
* @ class describe
*/

#ifndef UNPACKER_BUILDER_H
#define UNPACKER_BUILDER_H


#include "dex_ir_header.h"

namespace dex_ir {

// Eagerly assign offsets based on the original offsets in the input dex file. If this is not done,
// dex_ir::Item::GetOffset will abort when reading uninitialized offsets.
    Header *DexIrBuilder(const art::DexFile &dex_file,
                         bool eagerly_assign_offsets);



}  // namespace dex_ir



#endif //UNPACKER_BUILDER_H
