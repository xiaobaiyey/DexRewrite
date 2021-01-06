//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_CLASS_DATA_H
#define BASE_CLASS_DATA_H

#include "item.h"
#include "field_item.h"
#include "method_item.h"

namespace dex_ir {
    class ClassDef;

    class ClassData : public Item {
    public:
        ClassData(FieldItemVector *static_fields,
                  FieldItemVector *instance_fields,
                  MethodItemVector *direct_methods,
                  MethodItemVector *virtual_methods)
                : static_fields_(static_fields),
                  instance_fields_(instance_fields),
                  direct_methods_(direct_methods),
                  virtual_methods_(virtual_methods) {}

        ~ClassData() override = default;

        FieldItemVector *StaticFields() { return static_fields_.get(); }

        FieldItemVector *InstanceFields() { return instance_fields_.get(); }

        MethodItemVector *DirectMethods() { return direct_methods_.get(); }

        MethodItemVector *VirtualMethods() { return virtual_methods_.get(); }



        // void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }
        void SetClassDef(ClassDef *&classdef) {
            classdef_ = classdef;
        }

        const ClassDef *GetClassDef() const {
            return classdef_;
        }


    private:
        std::unique_ptr<FieldItemVector> static_fields_;
        std::unique_ptr<FieldItemVector> instance_fields_;
        std::unique_ptr<MethodItemVector> direct_methods_;
        std::unique_ptr<MethodItemVector> virtual_methods_;
        ClassDef *classdef_;
        DISALLOW_COPY_AND_ASSIGN(ClassData);
    };


}

#endif //BASE_CLASS_DATA_H
