//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_STRINGID_H
#define WAAPOLICY_DEX_IR_STRINGID_H

#include "dex_ir_item.h"
#include "dex_ir.h"
namespace dex_ir {

    class StringId : public IndexedItem {
    public:
        explicit StringId(StringData *string_data);

        ~StringId() override {}

        static size_t ItemSize() { return kStringIdItemSize; }

        const char *Data() const;

        StringData *DataItem() const;

    private:
        StringData *string_data_;
        DISALLOW_COPY_AND_ASSIGN(StringId);
    };
}
#endif //WAAPOLICY_DEX_IR_STRINGID_H
