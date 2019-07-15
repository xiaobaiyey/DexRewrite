/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019-04-18 21:57
* @ class describe
*/

#ifndef UNPACKER_DEXIRUTIL_H
#define UNPACKER_DEXIRUTIL_H

#include <string>
#include "dex_ir.h"
namespace dex_ir {
    class DexIRUtil {
    public:
        std::string GetSignatureForProtoId(const ProtoId* proto);
        std::string DescriptorToDotWrapper(const char* descriptor);
        std::string DescriptorToDot(const char* descriptor);
    };
}


#endif //UNPACKER_DEXIRUTIL_H
