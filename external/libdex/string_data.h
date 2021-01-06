//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_STRING_DATA_H
#define BASE_STRING_DATA_H

#include "item.h"
#include <libbase/leb128.h>
#include "dex/utf.h"
#include <libbase/stl_util.h>

namespace dex_ir {
    class StringData : public Item {
    public:
        explicit StringData(const char *data);

        const char *Data() const;
        bool SetData(const char *data);
    private:
        base::UniqueCPtr<const char> data_;
        DISALLOW_COPY_AND_ASSIGN(StringData);
    };
}


#endif //BASE_STRING_DATA_H
