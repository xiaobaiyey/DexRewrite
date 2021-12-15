//
// Created by xiaob on 2020/1/2.
//
#include "annotation.h"

namespace dex_ir {

    EncodedValue::EncodedValue(uint8_t type) : type_(type) {}

    int8_t EncodedValue::Type() const { return type_; }

    void EncodedValue::SetBoolean(bool z) { u_.bool_val_ = z; }

    void EncodedValue::SetByte(int8_t b) { u_.byte_val_ = b; }

    void EncodedValue::SetShort(int16_t s) { u_.short_val_ = s; }

    void EncodedValue::SetChar(uint16_t c) { u_.char_val_ = c; }

    void EncodedValue::SetInt(int32_t i) { u_.int_val_ = i; }

    void EncodedValue::SetLong(int64_t l) { u_.long_val_ = l; }

    void EncodedValue::SetFloat(float f) { u_.float_val_ = f; }

    void EncodedValue::SetDouble(double d) { u_.double_val_ = d; }

    void EncodedValue::SetStringId(StringId *string_id) { u_.string_val_ = string_id; }

    void EncodedValue::SetTypeId(TypeId *type_id) { u_.type_val_ = type_id; }

    void EncodedValue::SetProtoId(ProtoId *proto_id) { u_.proto_val_ = proto_id; }

    void EncodedValue::SetFieldId(FieldId *field_id) { u_.field_val_ = field_id; }

    void EncodedValue::SetMethodId(MethodId *method_id) { u_.method_val_ = method_id; }

    void EncodedValue::SetMethodHandle(MethodHandleItem *method_handle) { u_.method_handle_val_ = method_handle; }

    void EncodedValue::SetEncodedArray(EncodedArrayItem *encoded_array) { encoded_array_.reset(encoded_array); }

    void EncodedValue::SetEncodedAnnotation(EncodedAnnotation *encoded_annotation) {
        encoded_annotation_.reset(encoded_annotation);
    }

    bool EncodedValue::GetBoolean() const { return u_.bool_val_; }

    int8_t EncodedValue::GetByte() const { return u_.byte_val_; }

    int16_t EncodedValue::GetShort() const { return u_.short_val_; }

    uint16_t EncodedValue::GetChar() const { return u_.char_val_; }

    int32_t EncodedValue::GetInt() const { return u_.int_val_; }

    int64_t EncodedValue::GetLong() const { return u_.long_val_; }

    float EncodedValue::GetFloat() const { return u_.float_val_; }

    double EncodedValue::GetDouble() const { return u_.double_val_; }

    StringId *EncodedValue::GetStringId() const { return u_.string_val_; }

    TypeId *EncodedValue::GetTypeId() const { return u_.type_val_; }

    ProtoId *EncodedValue::GetProtoId() const { return u_.proto_val_; }

    FieldId *EncodedValue::GetFieldId() const { return u_.field_val_; }

    MethodId *EncodedValue::GetMethodId() const { return u_.method_val_; }

    MethodHandleItem *EncodedValue::GetMethodHandle() const { return u_.method_handle_val_; }

    EncodedArrayItem *EncodedValue::GetEncodedArray() const { return encoded_array_.get(); }

    EncodedAnnotation *EncodedValue::GetEncodedAnnotation() const { return encoded_annotation_.get(); }

    EncodedAnnotation *EncodedValue::ReleaseEncodedAnnotation() { return encoded_annotation_.release(); }

    EncodedValue::~EncodedValue() {

    }
}
