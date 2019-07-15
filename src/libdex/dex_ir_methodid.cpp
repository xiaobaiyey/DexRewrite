/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 10:02 PM
* @ class describe
*/


#include <dex/dex_ir_methodid.h>

namespace dex_ir {

    MethodId::MethodId(const TypeId *klass, const ProtoId *proto, const StringId *name)
            : class_(klass), proto_(proto), name_(name) { size_ = kMethodIdItemSize; }

    const TypeId *MethodId::Class() const {
        return class_;
    }

    const ProtoId *MethodId::Proto() const {
        return proto_;
    }

    const StringId *MethodId::Name() const {
        return name_;
    }
}
