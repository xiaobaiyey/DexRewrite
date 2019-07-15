//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_STRINGDATA_H
#define WAAPOLICY_DEX_IR_STRINGDATA_H

#include <stdlib.h>
#include <memory>
#include "dex_ir_item.h"

namespace dex_ir {
    // Deleter using free() for use with std::unique_ptr<>. See also UniqueCPtr<> below.
    struct FreeDelete {
        // NOTE: Deleting a const object is valid but free() takes a non-const pointer.
        void operator()(const void *ptr) const {
            free(const_cast<void *>(ptr));
        }
    };

// Alias for std::unique_ptr<> that uses the C function free() to delete objects.
    template<typename T>
    using UniqueCPtr = std::unique_ptr<T, FreeDelete>;

    class StringData : public Item {
    public:
        explicit StringData(const char *data);

        const char *Data() const;

        bool SetData(const char *data);

    private:
        UniqueCPtr<const char> data_;

        DISALLOW_COPY_AND_ASSIGN(StringData);
    };
}

#endif //WAAPOLICY_DEX_IR_STRINGDATA_H
