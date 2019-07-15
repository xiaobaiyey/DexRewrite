/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 11:52 PM
* @ class describe
*/

#ifndef UNPACKER_CLASSDATA_H
#define UNPACKER_CLASSDATA_H

#include <vector>
#include "dex_ir_fieid_item.h"
#include "dex_ir_method_item.h"
#include "dex_ir_item.h"
namespace dex_ir {
    class ClassData : public Item {
    public:
        ClassData(FieldItemVector *static_fields,
                  FieldItemVector *instance_fields,
                  MethodItemVector *direct_methods,
                  MethodItemVector *virtual_methods);

        ~ClassData() override = default;

        FieldItemVector *StaticFields() { return static_fields_.get(); }

        FieldItemVector *InstanceFields() { return instance_fields_.get(); }

        MethodItemVector *DirectMethods() { return direct_methods_.get(); }

        MethodItemVector *VirtualMethods() { return virtual_methods_.get(); }

        void SetClassDef(ClassDef *&classdef) {
            classdef_ = classdef;
        }


        ClassDef *GetClassDef() {
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


#endif //UNPACKER_CLASSDATA_H
