//
// Created by xiaob on 2020/1/1.
//
#include "hiddenapi_class_data.h"
#include "header.h"

namespace dex_ir {
    uint32_t
    HiddenapiClassData::GetFlags(Header *header, ClassDef *class_def, const Item *field_or_method_item) {
        DCHECK(header != nullptr);
        DCHECK(class_def != nullptr);
        return (header->HiddenapiClassDatas().Empty())
               ? 0u
               : header->HiddenapiClassDatas()[class_def->GetIndex()]->GetFlags(field_or_method_item);
    }
}

