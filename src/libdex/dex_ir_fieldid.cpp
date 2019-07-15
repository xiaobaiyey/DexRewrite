/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:58 PM
* @ class describe
*/


#include <dex/dex_ir_fieldid.h>

namespace dex_ir {

    FieldId::FieldId(const TypeId *klass, const TypeId *type, const StringId *name) : class_(
            klass), type_(type), name_(name) { size_ = kFieldIdItemSize; }

    const TypeId *FieldId::Class() const {
        return class_;
    }

    const TypeId *FieldId::Type() const {
        return type_;
    }

    const StringId *FieldId::Name() const {
        return name_;
    }
}

