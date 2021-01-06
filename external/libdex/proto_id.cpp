//
// Created by xiaob on 2020/1/1.
//

#include "proto_id.h"

namespace dex_ir {

    ProtoId::ProtoId(const StringId *shorty, const TypeId *return_type, TypeList *parameters) :
            shorty_(shorty),
            return_type_(return_type),
            parameters_(parameters) { size_ = kProtoIdItemSize; }

    ProtoId::~ProtoId() {

    }

    size_t ProtoId::ItemSize() { return kProtoIdItemSize; }

    const StringId *ProtoId::Shorty() const { return shorty_; }

    const TypeId *ProtoId::ReturnType() const { return return_type_; }

    const TypeList *ProtoId::Parameters() const { return parameters_; }

    std::string ProtoId::GetSignatureForProtoId() const {
        std::string result("(");
        const dex_ir::TypeList *type_list = Parameters();
        if (type_list != nullptr) {
            for (const dex_ir::TypeId *type_id : *type_list->GetTypeList()) {
                result += type_id->GetStringId()->Data();
            }
        }
        result += ")";
        result += ReturnType()->GetStringId()->Data();
        return result;
    }

}