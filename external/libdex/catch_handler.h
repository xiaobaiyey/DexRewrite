//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_CATCH_HANDLER_H
#define BASE_CATCH_HANDLER_H

#include <stdint.h>
#include "type_addr_pair.h"
namespace dex_ir{

    class CatchHandler {
    public:
        explicit CatchHandler(bool catch_all, uint16_t list_offset, TypeAddrPairVector* handlers)
                : catch_all_(catch_all), list_offset_(list_offset), handlers_(handlers) { }

        bool HasCatchAll() const { return catch_all_; }
        uint16_t GetListOffset() const { return list_offset_; }
        TypeAddrPairVector* GetHandlers() const { return handlers_.get(); }

    private:
        bool catch_all_;
        uint16_t list_offset_;
        std::unique_ptr<TypeAddrPairVector> handlers_;

        DISALLOW_COPY_AND_ASSIGN(CatchHandler);
    };

    using CatchHandlerVector = std::vector<std::unique_ptr<const CatchHandler>>;
}

#endif //BASE_CATCH_HANDLER_H
