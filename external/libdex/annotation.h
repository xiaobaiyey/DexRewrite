//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_ANNOTATION_H
#define BASE_ANNOTATION_H

#include <stdint.h>
#include "type_id.h"
#include "string_id.h"
#include "proto_id.h"
#include "field_id.h"
#include "method_id.h"
#include "method_handle_item.h"

namespace dex_ir {
    class EncodedArrayItem;

    class EncodedAnnotation;

    class EncodedValue {
    public:
        explicit EncodedValue(uint8_t type);

        ~EncodedValue();

        int8_t Type() const;

        void SetBoolean(bool z);

        void SetByte(int8_t b);

        void SetShort(int16_t s);

        void SetChar(uint16_t c);

        void SetInt(int32_t i);

        void SetLong(int64_t l);

        void SetFloat(float f);

        void SetDouble(double d);

        void SetStringId(StringId *string_id);

        void SetTypeId(TypeId *type_id);

        void SetProtoId(ProtoId *proto_id);

        void SetFieldId(FieldId *field_id);

        void SetMethodId(MethodId *method_id);

        void SetMethodHandle(MethodHandleItem *method_handle);

        void SetEncodedArray(EncodedArrayItem *encoded_array);

        void SetEncodedAnnotation(EncodedAnnotation *encoded_annotation);

        bool GetBoolean() const;

        int8_t GetByte() const;

        int16_t GetShort() const;

        uint16_t GetChar() const;

        int32_t GetInt() const;

        int64_t GetLong() const;

        float GetFloat() const;

        double GetDouble() const;

        StringId *GetStringId() const;

        TypeId *GetTypeId() const;

        ProtoId *GetProtoId() const;

        FieldId *GetFieldId() const;

        MethodId *GetMethodId() const;

        MethodHandleItem *GetMethodHandle() const;

        EncodedArrayItem *GetEncodedArray() const;

        EncodedAnnotation *GetEncodedAnnotation() const;

        EncodedAnnotation *ReleaseEncodedAnnotation();

    private:
        uint8_t type_;
        union {
            bool bool_val_;
            int8_t byte_val_;
            int16_t short_val_;
            uint16_t char_val_;
            int32_t int_val_;
            int64_t long_val_;
            float float_val_;
            double double_val_;
            StringId *string_val_;
            TypeId *type_val_;
            ProtoId *proto_val_;
            FieldId *field_val_;
            MethodId *method_val_;
            MethodHandleItem *method_handle_val_;
        } u_;
        std::unique_ptr<EncodedArrayItem> encoded_array_;
        std::unique_ptr<EncodedAnnotation> encoded_annotation_;
        DISALLOW_COPY_AND_ASSIGN(EncodedValue);
    };

    using EncodedValueVector = std::vector<std::unique_ptr<EncodedValue>>;


    class AnnotationElement {
    public:
        AnnotationElement(StringId *name, EncodedValue *value) : name_(name), value_(value) {}

        StringId *GetName() const { return name_; }

        EncodedValue *GetValue() const { return value_.get(); }

    private:
        StringId *name_;
        std::unique_ptr<EncodedValue> value_;

        DISALLOW_COPY_AND_ASSIGN(AnnotationElement);
    };

    using AnnotationElementVector = std::vector<std::unique_ptr<AnnotationElement>>;

    class EncodedAnnotation {
    public:
        EncodedAnnotation(TypeId *type, AnnotationElementVector *elements)
                : type_(type), elements_(elements) {}

        TypeId *GetType() const { return type_; }

        AnnotationElementVector *GetAnnotationElements() const { return elements_.get(); }

    private:
        TypeId *type_;
        std::unique_ptr<AnnotationElementVector> elements_;

        DISALLOW_COPY_AND_ASSIGN(EncodedAnnotation);
    };

    class EncodedArrayItem : public Item {
    public:
        explicit EncodedArrayItem(EncodedValueVector *encoded_values)
                : encoded_values_(encoded_values) {}

        EncodedValueVector *GetEncodedValues() const { return encoded_values_.get(); }

    private:
        std::unique_ptr<EncodedValueVector> encoded_values_;
        DISALLOW_COPY_AND_ASSIGN(EncodedArrayItem);
    };

    class AnnotationItem : public Item {
    public:
        AnnotationItem(uint8_t visibility, EncodedAnnotation *annotation)
                : visibility_(visibility), annotation_(annotation) {}

        uint8_t GetVisibility() const { return visibility_; }

        EncodedAnnotation *GetAnnotation() const { return annotation_.get(); }

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        uint8_t visibility_;
        std::unique_ptr<EncodedAnnotation> annotation_;

        DISALLOW_COPY_AND_ASSIGN(AnnotationItem);
    };

    class AnnotationSetItem : public Item {
    public:
        explicit AnnotationSetItem(std::vector<AnnotationItem *> *items) : items_(items) {
            size_ = sizeof(uint32_t) + items->size() * sizeof(uint32_t);
        }

        ~AnnotationSetItem() override {}

        std::vector<AnnotationItem *> *GetItems() { return items_.get(); }

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        std::unique_ptr<std::vector<AnnotationItem *>> items_;

        DISALLOW_COPY_AND_ASSIGN(AnnotationSetItem);
    };

    class AnnotationSetRefList : public Item {
    public:
        explicit AnnotationSetRefList(std::vector<AnnotationSetItem *> *items) : items_(items) {
            size_ = sizeof(uint32_t) + items->size() * sizeof(uint32_t);
        }

        ~AnnotationSetRefList() override {}

        std::vector<AnnotationSetItem *> *GetItems() { return items_.get(); }

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        std::unique_ptr<std::vector<AnnotationSetItem *>> items_;  // Elements of vector can be nullptr.
        DISALLOW_COPY_AND_ASSIGN(AnnotationSetRefList);
    };


    class FieldAnnotation {
    public:
        FieldAnnotation(FieldId *field_id, AnnotationSetItem *annotation_set_item)
                : field_id_(field_id), annotation_set_item_(annotation_set_item) {}

        FieldId *GetFieldId() const { return field_id_; }

        AnnotationSetItem *GetAnnotationSetItem() const { return annotation_set_item_; }

    private:
        FieldId *field_id_;
        AnnotationSetItem *annotation_set_item_;
        DISALLOW_COPY_AND_ASSIGN(FieldAnnotation);
    };

    using FieldAnnotationVector = std::vector<std::unique_ptr<FieldAnnotation>>;

    class MethodAnnotation {
    public:
        MethodAnnotation(MethodId *method_id, AnnotationSetItem *annotation_set_item)
                : method_id_(method_id), annotation_set_item_(annotation_set_item) {}

        MethodId *GetMethodId() const { return method_id_; }

        AnnotationSetItem *GetAnnotationSetItem() const { return annotation_set_item_; }

    private:
        MethodId *method_id_;
        AnnotationSetItem *annotation_set_item_;

        DISALLOW_COPY_AND_ASSIGN(MethodAnnotation);
    };

    using MethodAnnotationVector = std::vector<std::unique_ptr<MethodAnnotation>>;

    class ParameterAnnotation {
    public:
        ParameterAnnotation(MethodId *method_id, AnnotationSetRefList *annotations)
                : method_id_(method_id), annotations_(annotations) {}

        MethodId *GetMethodId() const { return method_id_; }

        AnnotationSetRefList *GetAnnotations() { return annotations_; }

    private:
        MethodId *method_id_;
        AnnotationSetRefList *annotations_;

        DISALLOW_COPY_AND_ASSIGN(ParameterAnnotation);
    };

    using ParameterAnnotationVector = std::vector<std::unique_ptr<ParameterAnnotation>>;

    class AnnotationsDirectoryItem : public Item {
    public:
        AnnotationsDirectoryItem(AnnotationSetItem *class_annotation,
                                 FieldAnnotationVector *field_annotations,
                                 MethodAnnotationVector *method_annotations,
                                 ParameterAnnotationVector *parameter_annotations)
                : class_annotation_(class_annotation),
                  field_annotations_(field_annotations),
                  method_annotations_(method_annotations),
                  parameter_annotations_(parameter_annotations) {}

        AnnotationSetItem *GetClassAnnotation() const { return class_annotation_; }

        FieldAnnotationVector *GetFieldAnnotations() { return field_annotations_.get(); }

        MethodAnnotationVector *GetMethodAnnotations() { return method_annotations_.get(); }

        ParameterAnnotationVector *GetParameterAnnotations() { return parameter_annotations_.get(); }

        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        AnnotationSetItem *class_annotation_;  // This can be nullptr.
        std::unique_ptr<FieldAnnotationVector> field_annotations_;  // This can be nullptr.
        std::unique_ptr<MethodAnnotationVector> method_annotations_;  // This can be nullptr.
        std::unique_ptr<ParameterAnnotationVector> parameter_annotations_;  // This can be nullptr.

        DISALLOW_COPY_AND_ASSIGN(AnnotationsDirectoryItem);
    };
}


#endif //BASE_ANNOTATION_H
