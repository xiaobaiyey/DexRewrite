//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_CLASS_DEF_H
#define BASE_CLASS_DEF_H

#include "indexed_item.h"
#include "type_id.h"
#include "type_list.h"
#include "string_id.h"
#include "class_data.h"
#include "annotation.h"

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

        ~ClassDef() override;

        static size_t ItemSize();

        const TypeId *ClassType() const;

        uint32_t GetAccessFlags() const;

        const TypeId *Superclass() const;

        const TypeList *Interfaces();

        uint32_t InterfacesOffset();

        const StringId *SourceFile() const;

        AnnotationsDirectoryItem *Annotations() const;

        ClassData *GetClassData();

        EncodedArrayItem *StaticValues();

        std::string getClassName() const;

        std::string getJavaClassName() const;

        std::string getPackage() const;

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


#endif //BASE_CLASS_DEF_H
