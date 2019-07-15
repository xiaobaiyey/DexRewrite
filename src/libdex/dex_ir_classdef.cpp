/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 11:55 PM
* @ class describe
*/


#include <dex/dex_ir_classdef.h>
#include "dex/dex_ir_typelist.h"
namespace dex_ir {

    ClassDef::ClassDef(const TypeId *class_type, uint32_t access_flags,
                       const TypeId *superclass, TypeList *interfaces,
                       const StringId *source_file, AnnotationsDirectoryItem *annotations,
                       EncodedArrayItem *static_values, ClassData *class_data) : class_type_(
            class_type),
                                                                                 access_flags_(
                                                                                         access_flags),
                                                                                 superclass_(
                                                                                         superclass),
                                                                                 interfaces_(
                                                                                         interfaces),
                                                                                 source_file_(
                                                                                         source_file),
                                                                                 annotations_(
                                                                                         annotations),
                                                                                 class_data_(
                                                                                         class_data),
                                                                                 static_values_(
                                                                                         static_values) { size_ = kClassDefItemSize; }

    uint32_t ClassDef::InterfacesOffset() {
        return interfaces_ == nullptr ? 0 : interfaces_->GetOffset();
    }
}