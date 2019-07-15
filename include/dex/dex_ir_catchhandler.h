/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:05 AM
* @ class describe
*/

#ifndef UNPACKER_CATCHHANDLER_H
#define UNPACKER_CATCHHANDLER_H

#include "dex_ir.h"
#include <base/macros.h>
#include "dex_ir_typeaddr_pair.h"

namespace dex_ir {
    class CatchHandler {
    public:
        explicit CatchHandler(bool catch_all, uint16_t list_offset,
                              TypeAddrPairVector *handlers);

        bool HasCatchAll() const { return catch_all_; }

        uint16_t GetListOffset() const { return list_offset_; }

        TypeAddrPairVector *GetHandlers() const { return handlers_.get(); }

    private:
        bool catch_all_;
        uint16_t list_offset_;
        std::unique_ptr<TypeAddrPairVector> handlers_;

        DISALLOW_COPY_AND_ASSIGN(CatchHandler);
    };

    using CatchHandlerVector = std::vector<std::unique_ptr<const CatchHandler>>;
}


#endif //UNPACKER_CATCHHANDLER_H
