/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 10:02 PM
* @ class describe
*/

#ifndef UNPACKER_METHODID_H
#define UNPACKER_METHODID_H


#include "dex_ir_item.h"
#include "dex_ir.h"

namespace dex_ir {
    class MethodId : public IndexedItem {
    public:
        MethodId(const TypeId *klass, const ProtoId *proto, const StringId *name);

        ~MethodId() override {}

        static size_t ItemSize() { return kMethodIdItemSize; }

        const TypeId *Class() const;

        const ProtoId *Proto() const;

        const StringId *Name() const;


    private:
        const TypeId *class_;
        const ProtoId *proto_;
        const StringId *name_;


        DISALLOW_COPY_AND_ASSIGN(MethodId);
    };
}


#endif //UNPACKER_METHODID_H
