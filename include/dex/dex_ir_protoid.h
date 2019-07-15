//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_PROTOID_H
#define WAAPOLICY_DEX_IR_PROTOID_H

#include "dex_ir_item.h"
#include "dex_ir.h"

namespace dex_ir {
    class ProtoId : public IndexedItem {
    public:
        ProtoId(const StringId *shorty, const TypeId *return_type, TypeList *parameters);

        ~ProtoId() override {}

        static size_t ItemSize() { return kProtoIdItemSize; }

        const StringId *Shorty() const;

        const TypeId *ReturnType() const;

        const TypeList *Parameters() const;

        std::string GetSignatureForProtoId() const;


    private:
        const StringId *shorty_;
        const TypeId *return_type_;
        TypeList *parameters_;  // This can be nullptr.

        DISALLOW_COPY_AND_ASSIGN(ProtoId);
    };
}


#endif //WAAPOLICY_DEX_IR_PROTOID_H
