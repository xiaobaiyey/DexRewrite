/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 11:52 PM
* @ class describe
*/


#include <dex/dex_ir_classdata.h>

namespace dex_ir {

    ClassData::ClassData(FieldItemVector *static_fields, FieldItemVector *instance_fields,
                         MethodItemVector *direct_methods, MethodItemVector *virtual_methods)
            : static_fields_(static_fields),
              instance_fields_(instance_fields),
              direct_methods_(direct_methods),
              virtual_methods_(virtual_methods) {}
}
