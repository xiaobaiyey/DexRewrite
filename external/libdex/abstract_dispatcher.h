//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_ABSTRACT_DISPATCHER_H
#define BASE_ABSTRACT_DISPATCHER_H

#include "libbase/macros.h"
#include <string>
#include <vector>

namespace dex_ir {
    // Visitor support
    class AbstractDispatcher {
    public:
        AbstractDispatcher() = default;

        virtual ~AbstractDispatcher() {}

        virtual void Dispatch(Header *header) = 0;

        virtual void Dispatch(const StringData *string_data) = 0;

        virtual void Dispatch(const StringId *string_id) = 0;

        virtual void Dispatch(const TypeId *type_id) = 0;

        virtual void Dispatch(const ProtoId *proto_id) = 0;

        virtual void Dispatch(const FieldId *field_id) = 0;

        virtual void Dispatch(const MethodId *method_id) = 0;

        virtual void Dispatch(const CallSiteId *call_site_id) = 0;

        virtual void Dispatch(const MethodHandleItem *method_handle_item) = 0;

        virtual void Dispatch(ClassData *class_data) = 0;

        virtual void Dispatch(ClassDef *class_def) = 0;

        virtual void Dispatch(FieldItem *field_item) = 0;

        virtual void Dispatch(MethodItem *method_item) = 0;

        virtual void Dispatch(EncodedArrayItem *array_item) = 0;

        virtual void Dispatch(CodeItem *code_item) = 0;

        virtual void Dispatch(TryItem *try_item) = 0;

        virtual void Dispatch(DebugInfoItem *debug_info_item) = 0;

        virtual void Dispatch(AnnotationItem *annotation_item) = 0;

        virtual void Dispatch(AnnotationSetItem *annotation_set_item) = 0;

        virtual void Dispatch(AnnotationSetRefList *annotation_set_ref_list) = 0;

        virtual void Dispatch(AnnotationsDirectoryItem *annotations_directory_item) = 0;

        virtual void Dispatch(HiddenapiClassData *hiddenapi_class_data) = 0;

        virtual void Dispatch(MapList *map_list) = 0;

        virtual void Dispatch(MapItem *map_item) = 0;

    private:
        DISALLOW_COPY_AND_ASSIGN(AbstractDispatcher);
    };



}


#endif //BASE_ABSTRACT_DISPATCHER_H
