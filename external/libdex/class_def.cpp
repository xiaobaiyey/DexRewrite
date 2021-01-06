//
// Created by xiaob on 2020/1/1.
//
#include "class_def.h"
#include "dex/descriptors_names.h"

namespace dex_ir {

    std::string ClassDef::getClassName() const {
        return this->class_type_->GetStringId()->Data();
    }

    std::string ClassDef::getJavaClassName() const {
        return libdex::DescriptorToDot(getClassName().c_str());
    }

    ClassDef::ClassDef(const TypeId *class_type, uint32_t access_flags, const TypeId *superclass, TypeList *interfaces,
                       const StringId *source_file, AnnotationsDirectoryItem *annotations,
                       EncodedArrayItem *static_values, ClassData *class_data) :
            class_type_(class_type),
            access_flags_(access_flags),
            superclass_(superclass),
            interfaces_(interfaces),
            source_file_(source_file),
            annotations_(annotations),
            class_data_(class_data),
            static_values_(static_values) { size_ = kClassDefItemSize; }

    ClassDef::~ClassDef() {
    }

    size_t ClassDef::ItemSize() {
        return kClassDefItemSize;
    }

    const TypeId *ClassDef::ClassType() const {
        return class_type_;
    }

    uint32_t ClassDef::GetAccessFlags() const {
        return access_flags_;
    }

    const TypeId *ClassDef::Superclass() const {
        return superclass_;
    }

    const TypeList *ClassDef::Interfaces() {
        return interfaces_;
    }

    uint32_t ClassDef::InterfacesOffset() {
        return interfaces_ == nullptr ? 0 : interfaces_->GetOffset();
    }

    const StringId *ClassDef::SourceFile() const {
        return source_file_;
    }

    AnnotationsDirectoryItem *ClassDef::Annotations() const {
        return annotations_;
    }

    ClassData *ClassDef::GetClassData() {
        return class_data_;
    }

    EncodedArrayItem *ClassDef::StaticValues() {
        return static_values_;
    }

    std::string ClassDef::getPackage() const {
        auto name = libdex::DescriptorToDot(getClassName().c_str());
        auto last_dot = name.find_last_of('.');
        return name.substr(0, last_dot);
    }
}
