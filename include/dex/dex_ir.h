//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_IR_H
#define WAAPOLICY_DEX_IR_H

#include <stdint.h>
#include <stdlib.h>
namespace dex_ir {
    class AnnotationItem;

    class AnnotationsDirectoryItem;

    class AnnotationSetItem;

    class AnnotationSetRefList;

    class CallSiteId;

    class ClassData;

    class ClassDef;

    class CodeItem;

    class DebugInfoItem;

    class EncodedAnnotation;

    class EncodedArrayItem;

    class EncodedValue;

    class FieldId;

    class FieldItem;

    class Header;

    class MapList;

    class MapItem;

    class MethodHandleItem;

    class MethodId;

    class MethodItem;

    class ParameterAnnotation;

    class ProtoId;

    class StringData;

    class StringId;

    class TryItem;

    class TypeId;

    class TypeList;

    class AnnotationElement;

    class TypeAddrPair;

    class CatchHandler;

    class FieldAnnotation;

    class MethodAnnotation;


    //每个项目所占的大小
    static constexpr size_t kHeaderItemSize = 112;
    static constexpr size_t kStringIdItemSize = 4;
    static constexpr size_t kTypeIdItemSize = 4;
    static constexpr size_t kProtoIdItemSize = 12;
    static constexpr size_t kFieldIdItemSize = 8;
    static constexpr size_t kMethodIdItemSize = 8;
    static constexpr size_t kClassDefItemSize = 32;
    static constexpr size_t kCallSiteIdItemSize = 4;
    static constexpr size_t kMethodHandleItemSize = 8;
}
#endif //WAAPOLICY_DEX_IR_H
