/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:58 PM
* @ class describe
*/

#ifndef UNPACKER_FIELDID_H
#define UNPACKER_FIELDID_H

#include "dex_ir.h"
#include "dex_ir_item.h"

namespace dex_ir {
    class FieldId : public IndexedItem {
    public:
        FieldId(const TypeId *klass, const TypeId *type, const StringId *name);

        ~FieldId() override {}

        static size_t ItemSize() { return kFieldIdItemSize; }

        const TypeId *Class() const;

        const TypeId *Type() const;

        const StringId *Name() const;

    private:
        const TypeId *class_;
        const TypeId *type_;
        const StringId *name_;

        DISALLOW_COPY_AND_ASSIGN(FieldId);
    };


}


#endif //UNPACKER_FIELDID_H
