//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_PROTO_ID_H
#define BASE_PROTO_ID_H

#include "indexed_item.h"
#include "type_list.h"
#include "string_id.h"

namespace dex_ir {
    class ProtoId : public IndexedItem {
    public:
        ProtoId(const StringId *shorty, const TypeId *return_type, TypeList *parameters);


        ~ProtoId() override;

        static size_t ItemSize();

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


#endif //BASE_PROTO_ID_H
