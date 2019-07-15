/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/18 10:01 AM
* @ class describe
*/

#ifndef UNPACKER_CATCHHANDLERITERATOR_H
#define UNPACKER_CATCHHANDLERITERATOR_H

#include <stdlib.h>
#include <stdint.h>
#include "libdex/base/DexFileTypes.h"
#include "macros.h"
#include "libdex/base/DexFile.h"

namespace libdex {

    class CodeItemDataAccessor;
    class CatchHandlerIterator {
    public:
        CatchHandlerIterator(const CodeItemDataAccessor &accessor, uint32_t address);

        CatchHandlerIterator(const CodeItemDataAccessor &accessor,
                             const DexFile::TryItem &try_item);

        explicit CatchHandlerIterator(const uint8_t *handler_data) {
            Init(handler_data);
        }

        TypeIndex GetHandlerTypeIndex() const {
            return handler_.type_idx_;
        }

        uint32_t GetHandlerAddress() const {
            return handler_.address_;
        }

        void Next();

        bool HasNext() const {
            return remaining_count_ != -1 || catch_all_;
        }

        // End of this set of catch blocks, convenience method to locate next set of catch blocks
        const uint8_t *EndDataPointer() const {
            CHECK(!HasNext());
            return current_data_;
        }

    private:
        void Init(const CodeItemDataAccessor &accessor, int32_t offset);

        void Init(const uint8_t *handler_data);

        struct CatchHandlerItem {
            TypeIndex type_idx_;  // type index of the caught exception type
            uint32_t address_;  // handler address
        } handler_;
        const uint8_t *current_data_;  // the current handler in dex file.
        int32_t remaining_count_;   // number of handlers not read.
        bool catch_all_;            // is there a handler that will catch all exceptions in case
        // that all typed handler does not match.
    };
}


#endif //UNPACKER_CATCHHANDLERITERATOR_H
