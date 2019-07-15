/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:20 PM
* @ class describe
*/

#ifndef UNPACKER_COLLECTIONBASE_H
#define UNPACKER_COLLECTIONBASE_H

#include "dex_ir.h"
#include <base/macros.h>
namespace dex_ir {

// Collections become owners of the objects added by moving them into unique pointers.
    template<class T> class CollectionBase {
    public:
        CollectionBase() = default;

        uint32_t GetOffset() const {
            return offset_;
        }
        void SetOffset(uint32_t new_offset) {
            offset_ = new_offset;
        }

    private:
        // Start out unassigned.
        uint32_t offset_ = 0u;

        DISALLOW_COPY_AND_ASSIGN(CollectionBase);
    };
}
#endif //UNPACKER_COLLECTIONBASE_H
