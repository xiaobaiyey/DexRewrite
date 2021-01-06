//
// Created by xiaob on 2020/1/2.
//

#include "dex_writer.h"
#include "method_item.h"
#include "header.h"
#include "dex/standard_dex_file.h"
#include "dex/compact_dex_file.h"
#include "libbase/logging.h"

namespace dex_ir {
    using namespace libdex;
    constexpr uint32_t DexWriter::kDataSectionAlignment;

    static size_t EncodeIntValue(int32_t value, uint8_t *buffer) {
        size_t length = 0;
        if (value >= 0) {
            while (value > 0x7f) {
                buffer[length++] = static_cast<uint8_t>(value);
                value >>= 8;
            }
        } else {
            while (value < -0x80) {
                buffer[length++] = static_cast<uint8_t>(value);
                value >>= 8;
            }
        }
        buffer[length++] = static_cast<uint8_t>(value);
        return length;
    }

    static size_t EncodeUIntValue(uint32_t value, uint8_t *buffer) {
        size_t length = 0;
        do {
            buffer[length++] = static_cast<uint8_t>(value);
            value >>= 8;
        } while (value != 0);
        return length;
    }

    static size_t EncodeLongValue(int64_t value, uint8_t *buffer) {
        size_t length = 0;
        if (value >= 0) {
            while (value > 0x7f) {
                buffer[length++] = static_cast<uint8_t>(value);
                value >>= 8;
            }
        } else {
            while (value < -0x80) {
                buffer[length++] = static_cast<uint8_t>(value);
                value >>= 8;
            }
        }
        buffer[length++] = static_cast<uint8_t>(value);
        return length;
    }

    union FloatUnion {
        float f_;
        uint32_t i_;
    };

    static size_t EncodeFloatValue(float value, uint8_t *buffer) {
        FloatUnion float_union;
        float_union.f_ = value;
        uint32_t int_value = float_union.i_;
        size_t index = 3;
        do {
            buffer[index--] = int_value >> 24;
            int_value <<= 8;
        } while (int_value != 0);
        return 3 - index;
    }

    union DoubleUnion {
        double d_;
        uint64_t l_;
    };

    static size_t EncodeDoubleValue(double value, uint8_t *buffer) {
        DoubleUnion double_union;
        double_union.d_ = value;
        uint64_t long_value = double_union.l_;
        size_t index = 7;
        do {
            buffer[index--] = long_value >> 56;
            long_value <<= 8;
        } while (long_value != 0);
        return 7 - index;
    }

    DexWriter::DexWriter(DexLayout *dex_layout, bool compute_offsets)
            : header_(dex_layout->GetHeader()),
              dex_layout_(dex_layout),
              compute_offsets_(compute_offsets) {}

    DexWriter::DexWriter(dex_ir::Header *header, bool compute_offsets) : header_(header),
                                                                         dex_layout_(nullptr),
                                                                         compute_offsets_(compute_offsets) {}

    void DexWriter::WriteEncodedValue(Stream *stream, dex_ir::EncodedValue *encoded_value) {
        size_t start = 0;
        size_t length;
        uint8_t buffer[8];
        int8_t type = encoded_value->Type();
        switch (type) {
            case libdex::DexFile::kDexAnnotationByte:
                length = EncodeIntValue(encoded_value->GetByte(), buffer);
                break;
            case libdex::DexFile::kDexAnnotationShort:
                length = EncodeIntValue(encoded_value->GetShort(), buffer);
                break;
            case libdex::DexFile::kDexAnnotationChar:
                length = EncodeUIntValue(encoded_value->GetChar(), buffer);
                break;
            case libdex::DexFile::kDexAnnotationInt:
                length = EncodeIntValue(encoded_value->GetInt(), buffer);
                break;
            case DexFile::kDexAnnotationLong:
                length = EncodeLongValue(encoded_value->GetLong(), buffer);
                break;
            case DexFile::kDexAnnotationFloat:
                length = EncodeFloatValue(encoded_value->GetFloat(), buffer);
                start = 4 - length;
                break;
            case DexFile::kDexAnnotationDouble:
                length = EncodeDoubleValue(encoded_value->GetDouble(), buffer);
                start = 8 - length;
                break;
            case DexFile::kDexAnnotationMethodType:
                length = EncodeUIntValue(encoded_value->GetProtoId()->GetIndex(), buffer);
                break;
            case DexFile::kDexAnnotationMethodHandle:
                length = EncodeUIntValue(encoded_value->GetMethodHandle()->GetIndex(), buffer);
                break;
            case DexFile::kDexAnnotationString:
                length = EncodeUIntValue(encoded_value->GetStringId()->GetIndex(), buffer);
                break;
            case DexFile::kDexAnnotationType:
                length = EncodeUIntValue(encoded_value->GetTypeId()->GetIndex(), buffer);
                break;
            case DexFile::kDexAnnotationField:
            case DexFile::kDexAnnotationEnum:
                length = EncodeUIntValue(encoded_value->GetFieldId()->GetIndex(), buffer);
                break;
            case DexFile::kDexAnnotationMethod:
                length = EncodeUIntValue(encoded_value->GetMethodId()->GetIndex(), buffer);
                break;
            case DexFile::kDexAnnotationArray:
                WriteEncodedValueHeader(stream, type, 0);
                WriteEncodedArray(stream, encoded_value->GetEncodedArray()->GetEncodedValues());
                return;
            case DexFile::kDexAnnotationAnnotation:
                WriteEncodedValueHeader(stream, type, 0);
                WriteEncodedAnnotation(stream, encoded_value->GetEncodedAnnotation());
                return;
            case DexFile::kDexAnnotationNull:
                WriteEncodedValueHeader(stream, type, 0);
                return;
            case DexFile::kDexAnnotationBoolean:
                WriteEncodedValueHeader(stream, type, encoded_value->GetBoolean() ? 1 : 0);
                return;
            default:
                return;
        }
        WriteEncodedValueHeader(stream, type, length - 1);
        stream->Write(buffer + start, length);
    }

    void DexWriter::WriteEncodedValueHeader(Stream *stream, int8_t value_type, size_t value_arg) {
        uint8_t buffer[1] = {static_cast<uint8_t>((value_arg << 5) | value_type)};
        stream->Write(buffer, sizeof(uint8_t));
    }

    void DexWriter::WriteEncodedArray(Stream *stream, dex_ir::EncodedValueVector *values) {
        stream->WriteUleb128(values->size());
        for (std::unique_ptr<dex_ir::EncodedValue> &value : *values) {
            WriteEncodedValue(stream, value.get());
        }
    }

    void DexWriter::WriteEncodedAnnotation(Stream *stream, dex_ir::EncodedAnnotation *annotation) {
        stream->WriteUleb128(annotation->GetType()->GetIndex());
        stream->WriteUleb128(annotation->GetAnnotationElements()->size());
        for (std::unique_ptr<dex_ir::AnnotationElement> &annotation_element :
                *annotation->GetAnnotationElements()) {
            stream->WriteUleb128(annotation_element->GetName()->GetIndex());
            WriteEncodedValue(stream, annotation_element->GetValue());
        }
    }

    void DexWriter::WriteEncodedFields(Stream *stream, dex_ir::FieldItemVector *fields) {
        uint32_t prev_index = 0;
        for (auto &field : *fields) {
            uint32_t index = field.GetFieldId()->GetIndex();
            stream->WriteUleb128(index - prev_index);
            stream->WriteUleb128(field.GetAccessFlags());
            prev_index = index;
        }
    }

    void DexWriter::WriteEncodedMethods(Stream *stream, dex_ir::MethodItemVector *methods) {
        uint32_t prev_index = 0;
        for (auto &method : *methods) {
            uint32_t index = method->GetMethodId()->GetIndex();
            uint32_t code_off = 0;
            if (method->isNative()) {
                code_off = 0;
            } else {
                code_off = method->GetCodeItem() == nullptr ? 0 : method->GetCodeItem()->GetOffset();
            }
            stream->WriteUleb128(index - prev_index);
            stream->WriteUleb128(method->GetAccessFlags());
            stream->WriteUleb128(code_off);
            prev_index = index;
        }
    }

// TODO: Refactor this to remove duplicated boiler plate. One way to do this is adding
// function that takes a CollectionVector<T> and uses overloading.
    void DexWriter::WriteStringIds(Stream *stream, bool reserve_only) {
        const uint32_t start = stream->Tell();
        for (auto &string_id : header_->StringIds()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeStringIdItem));
            if (reserve_only) {
                stream->Skip(string_id->GetSize());
            } else {
                uint32_t string_data_off = string_id->DataItem()->GetOffset();
                stream->Write(&string_data_off, string_id->GetSize());
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->StringIds().SetOffset(start);
        }
    }

    void DexWriter::WriteStringData(Stream *stream, dex_ir::StringData *string_data) {
        ProcessOffset(stream, string_data);
        stream->AlignTo(SectionAlignment(DexFile::kDexTypeStringDataItem));
        stream->WriteUleb128(CountModifiedUtf8Chars(string_data->Data()));
        stream->Write(string_data->Data(), strlen(string_data->Data()));
        // Skip null terminator (already zeroed out, no need to write).
        stream->Skip(1);
    }

    void DexWriter::WriteStringDatas(Stream *stream) {
        const uint32_t start = stream->Tell();
        for (auto &string_data : header_->StringDatas()) {
            WriteStringData(stream, string_data.get());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->StringDatas().SetOffset(start);
        }
    }

    void DexWriter::WriteTypeIds(Stream *stream) {
        uint32_t descriptor_idx[1];
        const uint32_t start = stream->Tell();
        for (auto &type_id : header_->TypeIds()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeTypeIdItem));
            ProcessOffset(stream, type_id.get());
            descriptor_idx[0] = type_id->GetStringId()->GetIndex();
            stream->Write(descriptor_idx, type_id->GetSize());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->TypeIds().SetOffset(start);
        }
    }

    void DexWriter::WriteTypeLists(Stream *stream) {
        uint32_t size[1];
        uint16_t list[1];
        const uint32_t start = stream->Tell();
        for (auto &type_list : header_->TypeLists()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeTypeList));
            size[0] = type_list->GetTypeList()->size();
            ProcessOffset(stream, type_list.get());
            stream->Write(size, sizeof(uint32_t));
            for (const dex_ir::TypeId *type_id : *type_list->GetTypeList()) {
                list[0] = type_id->GetIndex();
                stream->Write(list, sizeof(uint16_t));
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->TypeLists().SetOffset(start);
        }
    }

    void DexWriter::WriteProtoIds(Stream *stream, bool reserve_only) {
        uint32_t buffer[3];
        const uint32_t start = stream->Tell();
        for (auto &proto_id : header_->ProtoIds()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeProtoIdItem));
            ProcessOffset(stream, proto_id.get());
            if (reserve_only) {
                stream->Skip(proto_id->GetSize());
            } else {
                buffer[0] = proto_id->Shorty()->GetIndex();
                buffer[1] = proto_id->ReturnType()->GetIndex();
                buffer[2] = proto_id->Parameters() == nullptr ? 0 : proto_id->Parameters()->GetOffset();
                stream->Write(buffer, proto_id->GetSize());
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->ProtoIds().SetOffset(start);
        }
    }

    void DexWriter::WriteFieldIds(Stream *stream) {
        uint16_t buffer[4];
        const uint32_t start = stream->Tell();
        for (auto &field_id : header_->FieldIds()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeFieldIdItem));
            ProcessOffset(stream, field_id.get());
            buffer[0] = field_id->Class()->GetIndex();
            buffer[1] = field_id->Type()->GetIndex();
            buffer[2] = field_id->Name()->GetIndex();
            buffer[3] = field_id->Name()->GetIndex() >> 16;
            stream->Write(buffer, field_id->GetSize());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->FieldIds().SetOffset(start);
        }
    }

    void DexWriter::WriteMethodIds(Stream *stream) {
        uint16_t buffer[4];
        const uint32_t start = stream->Tell();
        for (auto &method_id : header_->MethodIds()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeMethodIdItem));
            ProcessOffset(stream, method_id.get());
            buffer[0] = method_id->Class()->GetIndex();
            buffer[1] = method_id->Proto()->GetIndex();
            buffer[2] = method_id->Name()->GetIndex();
            buffer[3] = method_id->Name()->GetIndex() >> 16;
            stream->Write(buffer, method_id->GetSize());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->MethodIds().SetOffset(start);
        }
    }

    void DexWriter::WriteEncodedArrays(Stream *stream) {
        const uint32_t start = stream->Tell();
        for (auto &encoded_array : header_->EncodedArrayItems()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeEncodedArrayItem));
            ProcessOffset(stream, encoded_array.get());
            WriteEncodedArray(stream, encoded_array->GetEncodedValues());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->EncodedArrayItems().SetOffset(start);
        }
    }

    void DexWriter::WriteAnnotations(Stream *stream) {
        uint8_t visibility[1];
        const uint32_t start = stream->Tell();
        for (auto &annotation : header_->AnnotationItems()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeAnnotationItem));
            visibility[0] = annotation->GetVisibility();
            ProcessOffset(stream, annotation.get());
            stream->Write(visibility, sizeof(uint8_t));
            WriteEncodedAnnotation(stream, annotation->GetAnnotation());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->AnnotationItems().SetOffset(start);
        }
    }

    void DexWriter::WriteAnnotationSets(Stream *stream) {
        uint32_t size[1];
        uint32_t annotation_off[1];
        const uint32_t start = stream->Tell();
        for (auto &annotation_set : header_->AnnotationSetItems()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeAnnotationSetItem));
            size[0] = annotation_set->GetItems()->size();
            ProcessOffset(stream, annotation_set.get());
            stream->Write(size, sizeof(uint32_t));
            for (dex_ir::AnnotationItem *annotation : *annotation_set->GetItems()) {
                annotation_off[0] = annotation->GetOffset();
                stream->Write(annotation_off, sizeof(uint32_t));
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->AnnotationSetItems().SetOffset(start);
        }
    }

    void DexWriter::WriteAnnotationSetRefs(Stream *stream) {
        uint32_t size[1];
        uint32_t annotations_off[1];
        const uint32_t start = stream->Tell();
        for (auto &annotation_set_ref : header_->AnnotationSetRefLists()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeAnnotationSetRefList));
            size[0] = annotation_set_ref->GetItems()->size();
            ProcessOffset(stream, annotation_set_ref.get());
            stream->Write(size, sizeof(uint32_t));
            for (dex_ir::AnnotationSetItem *annotation_set : *annotation_set_ref->GetItems()) {
                annotations_off[0] = annotation_set == nullptr ? 0 : annotation_set->GetOffset();
                stream->Write(annotations_off, sizeof(uint32_t));
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->AnnotationSetRefLists().SetOffset(start);
        }
    }

    void DexWriter::WriteAnnotationsDirectories(Stream *stream) {
        uint32_t directory_buffer[4];
        uint32_t annotation_buffer[2];
        const uint32_t start = stream->Tell();
        for (auto &annotations_directory : header_->AnnotationsDirectoryItems()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeAnnotationsDirectoryItem));
            ProcessOffset(stream, annotations_directory.get());
            directory_buffer[0] = annotations_directory->GetClassAnnotation() == nullptr ? 0 :
                                  annotations_directory->GetClassAnnotation()->GetOffset();
            directory_buffer[1] = annotations_directory->GetFieldAnnotations() == nullptr ? 0 :
                                  annotations_directory->GetFieldAnnotations()->size();
            directory_buffer[2] = annotations_directory->GetMethodAnnotations() == nullptr ? 0 :
                                  annotations_directory->GetMethodAnnotations()->size();
            directory_buffer[3] = annotations_directory->GetParameterAnnotations() == nullptr ? 0 :
                                  annotations_directory->GetParameterAnnotations()->size();
            stream->Write(directory_buffer, 4 * sizeof(uint32_t));
            if (annotations_directory->GetFieldAnnotations() != nullptr) {
                for (std::unique_ptr<dex_ir::FieldAnnotation> &field :
                        *annotations_directory->GetFieldAnnotations()) {
                    annotation_buffer[0] = field->GetFieldId()->GetIndex();
                    annotation_buffer[1] = field->GetAnnotationSetItem()->GetOffset();
                    stream->Write(annotation_buffer, 2 * sizeof(uint32_t));
                }
            }
            if (annotations_directory->GetMethodAnnotations() != nullptr) {
                for (std::unique_ptr<dex_ir::MethodAnnotation> &method :
                        *annotations_directory->GetMethodAnnotations()) {
                    annotation_buffer[0] = method->GetMethodId()->GetIndex();
                    annotation_buffer[1] = method->GetAnnotationSetItem()->GetOffset();
                    stream->Write(annotation_buffer, 2 * sizeof(uint32_t));
                }
            }
            if (annotations_directory->GetParameterAnnotations() != nullptr) {
                for (std::unique_ptr<dex_ir::ParameterAnnotation> &parameter :
                        *annotations_directory->GetParameterAnnotations()) {
                    annotation_buffer[0] = parameter->GetMethodId()->GetIndex();
                    annotation_buffer[1] = parameter->GetAnnotations()->GetOffset();
                    stream->Write(annotation_buffer, 2 * sizeof(uint32_t));
                }
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->AnnotationsDirectoryItems().SetOffset(start);
        }
    }

    void DexWriter::WriteHiddenapiClassData(Stream *stream) {
        if (header_->HiddenapiClassDatas().Empty()) {
            return;
        }
        DCHECK_EQ(header_->HiddenapiClassDatas().Size(), header_->ClassDefs().Size());

        stream->AlignTo(SectionAlignment(DexFile::kDexTypeHiddenapiClassData));
        ProcessOffset(stream, &header_->HiddenapiClassDatas());
        const uint32_t start = stream->Tell();

        // Compute offsets for each class def and write the header.
        // data_header[0]: total size of the section
        // data_header[i + 1]: offset of class def[i] from the beginning of the section,
        //                     or zero if no data
        std::vector<uint32_t> data_header(header_->ClassDefs().Size() + 1, 0);
        data_header[0] = sizeof(uint32_t) * (header_->ClassDefs().Size() + 1);
        for (uint32_t i = 0; i < header_->ClassDefs().Size(); ++i) {
            uint32_t item_size = header_->HiddenapiClassDatas()[i]->ItemSize();
            data_header[i + 1] = item_size == 0u ? 0 : data_header[0];
            data_header[0] += item_size;
        }
        stream->Write(data_header.data(), sizeof(uint32_t) * data_header.size());

        // Write class data streams.
        for (uint32_t i = 0; i < header_->ClassDefs().Size(); ++i) {
            dex_ir::ClassDef *class_def = header_->ClassDefs()[i];
            const auto &item = header_->HiddenapiClassDatas()[i];
            DCHECK(item->GetClassDef() == class_def);

            if (data_header[i + 1] != 0u) {
                dex_ir::ClassData *class_data = class_def->GetClassData();
                DCHECK(class_data != nullptr);
                DCHECK_EQ(data_header[i + 1], stream->Tell() - start);
                for (const dex_ir::FieldItem &field : *class_data->StaticFields()) {
                    stream->WriteUleb128(item->GetFlags(&field));
                }
                for (const dex_ir::FieldItem &field : *class_data->InstanceFields()) {
                    stream->WriteUleb128(item->GetFlags(&field));
                }
                for (const auto &method : *class_data->DirectMethods()) {
                    stream->WriteUleb128(item->GetFlags(method.get()));
                }
                for (const auto &method : *class_data->VirtualMethods()) {
                    stream->WriteUleb128(item->GetFlags(method.get()));
                }
            }
        }
        DCHECK_EQ(stream->Tell() - start, data_header[0]);

        if (compute_offsets_ && start != stream->Tell()) {
            header_->HiddenapiClassDatas().SetOffset(start);
        }
    }

    void DexWriter::WriteDebugInfoItem(Stream *stream, dex_ir::DebugInfoItem *debug_info) {
        stream->AlignTo(SectionAlignment(DexFile::kDexTypeDebugInfoItem));
        ProcessOffset(stream, debug_info);
        stream->Write(debug_info->GetDebugInfo(), debug_info->GetDebugInfoSize());
    }

    void DexWriter::WriteDebugInfoItems(Stream *stream) {
        const uint32_t start = stream->Tell();
        for (auto &debug_info : header_->DebugInfoItems()) {
            WriteDebugInfoItem(stream, debug_info.get());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->DebugInfoItems().SetOffset(start);
        }
    }

    void DexWriter::WriteCodeItemPostInstructionData(Stream *stream,
                                                     dex_ir::CodeItem *code_item,
                                                     bool reserve_only) {
        if (code_item->TriesSize() != 0) {
            stream->AlignTo(dex::TryItem::kAlignment);
            // Write try items.
            for (std::unique_ptr<const dex_ir::TryItem> &try_item : *code_item->Tries()) {
                libdex::dex::TryItem disk_try_item;
                if (!reserve_only) {
                    disk_try_item.start_addr_ = try_item->StartAddr();
                    disk_try_item.insn_count_ = try_item->InsnCount();
                    disk_try_item.handler_off_ = try_item->GetHandlers()->GetListOffset();
                }
                stream->Write(&disk_try_item, sizeof(disk_try_item));
            }
            // Leave offset pointing to the end of the try items.
            const size_t offset = stream->Tell();
            size_t max_offset = offset + stream->WriteUleb128(code_item->Handlers()->size());
            for (std::unique_ptr<const dex_ir::CatchHandler> &handlers : *code_item->Handlers()) {
                stream->Seek(offset + handlers->GetListOffset());
                uint32_t size = handlers->HasCatchAll() ? (handlers->GetHandlers()->size() - 1) * -1 :
                                handlers->GetHandlers()->size();
                stream->WriteSleb128(size);
                for (std::unique_ptr<const dex_ir::TypeAddrPair> &handler : *handlers->GetHandlers()) {
                    if (handler->GetTypeId() != nullptr) {
                        stream->WriteUleb128(handler->GetTypeId()->GetIndex());
                    }
                    stream->WriteUleb128(handler->GetAddress());
                }
                // TODO: Clean this up to write the handlers in address order.
                max_offset = std::max(max_offset, stream->Tell());
            }
            stream->Seek(max_offset);
        }
    }

    void DexWriter::WriteCodeItem(Stream *stream,
                                  dex_ir::CodeItem *code_item,
                                  bool reserve_only) {
        DCHECK(code_item != nullptr);
        const uint32_t start_offset = stream->Tell();
        stream->AlignTo(SectionAlignment(DexFile::kDexTypeCodeItem));
        ProcessOffset(stream, code_item);

        libdex::StandardDexFile::CodeItem disk_code_item;
        if (!reserve_only) {
            disk_code_item.registers_size_ = code_item->RegistersSize();
            disk_code_item.ins_size_ = code_item->InsSize();
            disk_code_item.outs_size_ = code_item->OutsSize();
            disk_code_item.tries_size_ = code_item->TriesSize();
            disk_code_item.debug_info_off_ = code_item->DebugInfo() == nullptr
                                             ? 0
                                             : code_item->DebugInfo()->GetOffset();
            disk_code_item.insns_size_in_code_units_ = code_item->InsnsSize();
        }
        // Avoid using sizeof so that we don't write the fake instruction array at the end of the code
        // item.
        stream->Write(&disk_code_item, OFFSETOF_MEMBER(StandardDexFile::CodeItem, insns_));
        // Write the instructions.
        stream->Write(code_item->Insns(), code_item->InsnsSize() * sizeof(uint16_t));
        // Write the post instruction data.
        WriteCodeItemPostInstructionData(stream, code_item, reserve_only);
        if (reserve_only) {
            stream->Clear(start_offset, stream->Tell() - start_offset);
        }
    }

    void DexWriter::WriteCodeItems(Stream *stream, bool reserve_only) {
        DexLayoutSection *code_section = nullptr;
        if (!reserve_only && dex_layout_ != nullptr) {
            code_section = &dex_layout_->GetSections().sections_[static_cast<size_t>(
                    DexLayoutSections::SectionType::kSectionTypeCode)];
        }
        const uint32_t start = stream->Tell();
        for (auto &code_item : header_->CodeItems()) {
            auto method_item = code_item->GetMethodItem();
            if (method_item->isNative()) {
                continue;
            }
            uint32_t start_offset = stream->Tell();
            WriteCodeItem(stream, code_item.get(), reserve_only);
            // Only add the section hotness info once.
        }

        if (compute_offsets_ && start != stream->Tell()) {
            header_->CodeItems().SetOffset(start);
        }
    }

    void DexWriter::WriteClassDefs(Stream *stream, bool reserve_only) {
        const uint32_t start = stream->Tell();
        uint32_t class_def_buffer[8];
        for (auto &class_def : header_->ClassDefs()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeClassDefItem));
            if (reserve_only) {
                stream->Skip(class_def->GetSize());
            } else {
                class_def_buffer[0] = class_def->ClassType()->GetIndex();
                class_def_buffer[1] = class_def->GetAccessFlags();
                class_def_buffer[2] = class_def->Superclass() == nullptr ? dex::kDexNoIndex :
                                      class_def->Superclass()->GetIndex();
                class_def_buffer[3] = class_def->InterfacesOffset();
                class_def_buffer[4] = class_def->SourceFile() == nullptr ? dex::kDexNoIndex :
                                      class_def->SourceFile()->GetIndex();
                class_def_buffer[5] = class_def->Annotations() == nullptr ? 0 :
                                      class_def->Annotations()->GetOffset();
                class_def_buffer[6] = class_def->GetClassData() == nullptr ? 0 :
                                      class_def->GetClassData()->GetOffset();
                class_def_buffer[7] = class_def->StaticValues() == nullptr ? 0 : class_def->StaticValues()->GetOffset();
                stream->Write(class_def_buffer, class_def->GetSize());
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->ClassDefs().SetOffset(start);
        }
    }

    void DexWriter::WriteClassDatas(Stream *stream) {
        const uint32_t start = stream->Tell();
        for (const std::unique_ptr<dex_ir::ClassData> &class_data :
                header_->ClassDatas()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeClassDataItem));
            ProcessOffset(stream, class_data.get());
            stream->WriteUleb128(class_data->StaticFields()->size());
            stream->WriteUleb128(class_data->InstanceFields()->size());
            stream->WriteUleb128(class_data->DirectMethods()->size());
            stream->WriteUleb128(class_data->VirtualMethods()->size());
            WriteEncodedFields(stream, class_data->StaticFields());
            WriteEncodedFields(stream, class_data->InstanceFields());
            WriteEncodedMethods(stream, class_data->DirectMethods());
            WriteEncodedMethods(stream, class_data->VirtualMethods());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->ClassDatas().SetOffset(start);
        }
    }

    void DexWriter::WriteCallSiteIds(Stream *stream, bool reserve_only) {
        const uint32_t start = stream->Tell();
        uint32_t call_site_off[1];
        for (auto &call_site_id : header_->CallSiteIds()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeCallSiteIdItem));
            if (reserve_only) {
                stream->Skip(call_site_id->GetSize());
            } else {
                call_site_off[0] = call_site_id->CallSiteItem()->GetOffset();
                stream->Write(call_site_off, call_site_id->GetSize());
            }
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->CallSiteIds().SetOffset(start);
        }
    }

    void DexWriter::WriteMethodHandles(Stream *stream) {
        const uint32_t start = stream->Tell();
        uint16_t method_handle_buff[4];
        for (auto &method_handle : header_->MethodHandleItems()) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeMethodHandleItem));
            method_handle_buff[0] = static_cast<uint16_t>(method_handle->GetMethodHandleType());
            method_handle_buff[1] = 0;  // unused.
            method_handle_buff[2] = method_handle->GetFieldOrMethodId()->GetIndex();
            method_handle_buff[3] = 0;  // unused.
            stream->Write(method_handle_buff, method_handle->GetSize());
        }
        if (compute_offsets_ && start != stream->Tell()) {
            header_->MethodHandleItems().SetOffset(start);
        }
    }

    void DexWriter::WriteMapItems(Stream *stream, MapItemQueue *queue) {
        // All the sections should already have been added.
        const uint32_t map_list_size = queue->size();
        stream->Write(&map_list_size, sizeof(map_list_size));
        while (!queue->empty()) {
            const MapItem &item = queue->top();
            dex::MapItem map_item;
            map_item.type_ = item.type_;
            map_item.size_ = item.size_;
            map_item.offset_ = item.offset_;
            map_item.unused_ = 0u;
            stream->Write(&map_item, sizeof(map_item));
            queue->pop();
        }
    }

    void DexWriter::GenerateAndWriteMapItems(Stream *stream) {
        MapItemQueue queue;

        // Header and index section.
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeHeaderItem, 1, 0));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeStringIdItem,
                                    header_->StringIds().Size(),
                                    header_->StringIds().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeTypeIdItem,
                                    header_->TypeIds().Size(),
                                    header_->TypeIds().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeProtoIdItem,
                                    header_->ProtoIds().Size(),
                                    header_->ProtoIds().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeFieldIdItem,
                                    header_->FieldIds().Size(),
                                    header_->FieldIds().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeMethodIdItem,
                                    header_->MethodIds().Size(),
                                    header_->MethodIds().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeClassDefItem,
                                    header_->ClassDefs().Size(),
                                    header_->ClassDefs().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeCallSiteIdItem,
                                    header_->CallSiteIds().Size(),
                                    header_->CallSiteIds().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeMethodHandleItem,
                                    header_->MethodHandleItems().Size(),
                                    header_->MethodHandleItems().GetOffset()));
        // Data section.
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeMapList, 1, header_->MapListOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeTypeList,
                                    header_->TypeLists().Size(),
                                    header_->TypeLists().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeAnnotationSetRefList,
                                    header_->AnnotationSetRefLists().Size(),
                                    header_->AnnotationSetRefLists().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeAnnotationSetItem,
                                    header_->AnnotationSetItems().Size(),
                                    header_->AnnotationSetItems().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeClassDataItem,
                                    header_->ClassDatas().Size(),
                                    header_->ClassDatas().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeCodeItem,
                                    header_->CodeItems().Size(),
                                    header_->CodeItems().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeStringDataItem,
                                    header_->StringDatas().Size(),
                                    header_->StringDatas().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeDebugInfoItem,
                                    header_->DebugInfoItems().Size(),
                                    header_->DebugInfoItems().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeAnnotationItem,
                                    header_->AnnotationItems().Size(),
                                    header_->AnnotationItems().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeEncodedArrayItem,
                                    header_->EncodedArrayItems().Size(),
                                    header_->EncodedArrayItems().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeAnnotationsDirectoryItem,
                                    header_->AnnotationsDirectoryItems().Size(),
                                    header_->AnnotationsDirectoryItems().GetOffset()));
        queue.AddIfNotEmpty(MapItem(DexFile::kDexTypeHiddenapiClassData,
                                    header_->HiddenapiClassDatas().Empty() ? 0u : 1u,
                                    header_->HiddenapiClassDatas().GetOffset()));
        WriteMapItems(stream, &queue);
    }

    void DexWriter::WriteHeader(Stream *stream) {
        StandardDexFile::Header header;
        if (CompactDexFile::IsMagicValid(header_->Magic())) {
            StandardDexFile::WriteMagic(header.magic_);
            // TODO: Should we write older versions based on the feature flags?
            StandardDexFile::WriteCurrentVersion(header.magic_);
        } else {
            // Standard dex -> standard dex, just reuse the same header.
            static constexpr size_t kMagicAndVersionLen =
                    StandardDexFile::kDexMagicSize + StandardDexFile::kDexVersionLen;
            std::copy_n(header_->Magic(), kMagicAndVersionLen, header.magic_);
        }
        header.checksum_ = header_->Checksum();
        std::copy_n(header_->Signature(), DexFile::kSha1DigestSize, header.signature_);
        header.file_size_ = header_->FileSize();
        header.header_size_ = GetHeaderSize();
        header.endian_tag_ = header_->EndianTag();
        header.link_size_ = header_->LinkSize();
        header.link_off_ = header_->LinkOffset();
        header.map_off_ = header_->MapListOffset();
        header.string_ids_size_ = header_->StringIds().Size();
        header.string_ids_off_ = header_->StringIds().GetOffset();
        header.type_ids_size_ = header_->TypeIds().Size();
        header.type_ids_off_ = header_->TypeIds().GetOffset();
        header.proto_ids_size_ = header_->ProtoIds().Size();
        header.proto_ids_off_ = header_->ProtoIds().GetOffset();
        header.field_ids_size_ = header_->FieldIds().Size();
        header.field_ids_off_ = header_->FieldIds().GetOffset();
        header.method_ids_size_ = header_->MethodIds().Size();
        header.method_ids_off_ = header_->MethodIds().GetOffset();
        header.class_defs_size_ = header_->ClassDefs().Size();
        header.class_defs_off_ = header_->ClassDefs().GetOffset();
        header.data_size_ = header_->DataSize();
        header.data_off_ = header_->DataOffset();

        CHECK_EQ(sizeof(header), GetHeaderSize());
        static_assert(sizeof(header) == 0x70, "Size doesn't match dex spec");
        stream->Seek(0);
        stream->Overwrite(reinterpret_cast<uint8_t *>(&header), sizeof(header));
    }

    size_t DexWriter::GetHeaderSize() const {
        return sizeof(StandardDexFile::Header);
    }

    bool DexWriter::Write(DexContainer *output, std::string *error_msg) {
        DCHECK(error_msg != nullptr);

        Stream stream_storage(output->GetMainSection());
        Stream *stream = &stream_storage;

        // Starting offset is right after the header.
        stream->Seek(GetHeaderSize());

        // Based on: https://source.android.com/devices/tech/dalvik/dex-format
        // Since the offsets may not be calculated already, the writing must be done in the correct order.
        const uint32_t string_ids_offset = stream->Tell();
        WriteStringIds(stream, /*reserve_only=*/ true);
        WriteTypeIds(stream);
        const uint32_t proto_ids_offset = stream->Tell();
        WriteProtoIds(stream, /*reserve_only=*/ true);
        WriteFieldIds(stream);
        WriteMethodIds(stream);
        const uint32_t class_defs_offset = stream->Tell();
        WriteClassDefs(stream, /*reserve_only=*/ true);
        const uint32_t call_site_ids_offset = stream->Tell();
        WriteCallSiteIds(stream, /*reserve_only=*/ true);
        WriteMethodHandles(stream);

        uint32_t data_offset_ = 0u;
        if (compute_offsets_) {
            // Data section.
            stream->AlignTo(kDataSectionAlignment);
            data_offset_ = stream->Tell();
        }

        // Write code item first to minimize the space required for encoded methods.
        // Reserve code item space since we need the debug offsets to actually write them.
        const uint32_t code_items_offset = stream->Tell();
        WriteCodeItems(stream, /*reserve_only=*/ true);
        // Write debug info section.
        WriteDebugInfoItems(stream);
        {
            // Actually write code items since debug info offsets are calculated now.
            Stream::ScopedSeek seek(stream, code_items_offset);
            WriteCodeItems(stream, /*reserve_only=*/ false);
        }

        WriteEncodedArrays(stream);
        WriteAnnotations(stream);
        WriteAnnotationSets(stream);
        WriteAnnotationSetRefs(stream);
        WriteAnnotationsDirectories(stream);
        WriteTypeLists(stream);
        WriteClassDatas(stream);
        WriteStringDatas(stream);
        WriteHiddenapiClassData(stream);

        // Write delayed id sections that depend on data sections.
        {
            Stream::ScopedSeek seek(stream, string_ids_offset);
            WriteStringIds(stream, /*reserve_only=*/ false);
        }
        {
            Stream::ScopedSeek seek(stream, proto_ids_offset);
            WriteProtoIds(stream, /*reserve_only=*/ false);
        }
        {
            Stream::ScopedSeek seek(stream, class_defs_offset);
            WriteClassDefs(stream, /*reserve_only=*/ false);
        }
        {
            Stream::ScopedSeek seek(stream, call_site_ids_offset);
            WriteCallSiteIds(stream, /*reserve_only=*/ false);
        }

        // Write the map list.
        if (compute_offsets_) {
            stream->AlignTo(SectionAlignment(DexFile::kDexTypeMapList));
            header_->SetMapListOffset(stream->Tell());
        } else {
            stream->Seek(header_->MapListOffset());
        }
        GenerateAndWriteMapItems(stream);
        stream->AlignTo(kDataSectionAlignment);

        // Map items are included in the data section.
        if (compute_offsets_) {
            header_->SetDataSize(stream->Tell() - data_offset_);
            if (header_->DataSize() != 0) {
                // Offset must be zero when the size is zero.
                header_->SetDataOffset(data_offset_);
            } else {
                header_->SetDataOffset(0u);
            }
        }

        // Write link data if it exists.
        const std::vector<uint8_t> &link_data = header_->LinkData();
        if (link_data.size() > 0) {
            CHECK_EQ(header_->LinkSize(), static_cast<uint32_t>(link_data.size()));
            if (compute_offsets_) {
                header_->SetLinkOffset(stream->Tell());
            } else {
                stream->Seek(header_->LinkOffset());
            }
            stream->Write(&link_data[0], link_data.size());
        }

        // Write header last.
        if (compute_offsets_) {
            header_->SetFileSize(stream->Tell());
        }
        WriteHeader(stream);


        header_->SetChecksum(DexFile::CalculateChecksum(stream->Begin(), header_->FileSize()));
        // Rewrite the header with the calculated checksum.
        WriteHeader(stream);


        // Trim the map to make it sized as large as the dex file.
        output->GetMainSection()->Resize(header_->FileSize());
        return true;
    }

    bool DexWriter::Output(DexLayout *dex_layout,
                           std::unique_ptr<DexContainer> *container,
                           bool compute_offsets,
                           std::string *error_msg) {
        CHECK(dex_layout != nullptr);
        std::unique_ptr<DexWriter> writer;
        writer.reset(new DexWriter(dex_layout, compute_offsets));

        DCHECK(container != nullptr);
        if (*container == nullptr) {
            *container = writer->CreateDexContainer();
        }
        return writer->Write(container->get(), error_msg);
    }

    void MapItemQueue::AddIfNotEmpty(const MapItem &item) {
        if (item.size_ != 0) {
            push(item);
        }
    }

    void DexWriter::ProcessOffset(Stream *stream, dex_ir::Item *item) {
        if (compute_offsets_) {
            item->SetOffset(stream->Tell());
        } else {
            // Not computing offsets, just use the one in the item.
            stream->Seek(item->GetOffset());
        }
    }

    void DexWriter::ProcessOffset(Stream *stream, dex_ir::CollectionBase *item) {
        if (compute_offsets_) {
            item->SetOffset(stream->Tell());
        } else {
            // Not computing offsets, just use the one in the item.
            stream->Seek(item->GetOffset());
        }
    }

    std::unique_ptr<DexContainer> DexWriter::CreateDexContainer() const {
        return std::unique_ptr<DexContainer>(new DexWriter::Container);
    }


    bool DexWriter::Output(dex_ir::Header *header, std::string outpath) {
        std::unique_ptr<DexWriter> writer;

        writer.reset(new DexWriter(header, true));
        std::unique_ptr<DexContainer> container = writer->CreateDexContainer();;
        std::string error_msg;
        bool write_ = writer->Write(container.get(), &error_msg);
        //LOGI("try write:%s", outpath.c_str());
        FILE *file_x = fopen(outpath.c_str(), "wb");
        uint8_t *data = container.get()->GetMainSection()->Begin();
        if (data == nullptr)
            return false;
        uint32_t len = container.get()->GetMainSection()->Size();
        CHECK_NE(len, 0);
        fwrite(data, 1, len, file_x);
        fclose(file_x);
        LOG(DEBUG) << "[+]wirte:" << outpath;
        //__android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, "[+]wirte:%s", outpath.c_str());
        return write_;
        return false;
    }


}