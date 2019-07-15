/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 11:55 PM
* @ class describe
*/

#ifndef UNPACKER_CLASSDEF_H
#define UNPACKER_CLASSDEF_H


#include "dex_ir_item.h"
#include "dex_ir.h"
#include "dex_file.h"

namespace libdex {
    class ClassDataItemIterator;
}
namespace dex_ir {
    class ClassDef : public IndexedItem {
    public:
        ClassDef(const TypeId *class_type,
                 uint32_t access_flags,
                 const TypeId *superclass,
                 TypeList *interfaces,
                 const StringId *source_file,
                 AnnotationsDirectoryItem *annotations,
                 EncodedArrayItem *static_values,
                 ClassData *class_data);


        ~ClassDef() override {}

        static size_t ItemSize() { return kClassDefItemSize; }

        const TypeId *ClassType() const { return class_type_; }

        uint32_t GetAccessFlags() const { return access_flags_; }

        const TypeId *Superclass() const { return superclass_; }

        const TypeList *Interfaces() { return interfaces_; }

        uint32_t InterfacesOffset();

        const StringId *SourceFile() const { return source_file_; }

        AnnotationsDirectoryItem *Annotations() const { return annotations_; }

        ClassData *GetClassData() { return class_data_; }

        EncodedArrayItem *StaticValues() { return static_values_; }

        MethodItem *GenerateMethodItem(Header &header, art::ClassDataItemIterator &cdii);


    private:
        const TypeId *class_type_;
        uint32_t access_flags_;
        const TypeId *superclass_;  // This can be nullptr.
        TypeList *interfaces_;  // This can be nullptr.
        const StringId *source_file_;  // This can be nullptr.
        AnnotationsDirectoryItem *annotations_;  // This can be nullptr.
        ClassData *class_data_;  // This can be nullptr.
        EncodedArrayItem *static_values_;  // This can be nullptr.

        DISALLOW_COPY_AND_ASSIGN(ClassDef);
    };
}

#endif //UNPACKER_CLASSDEF_H
