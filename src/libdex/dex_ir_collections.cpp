/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:25 PM
* @ class describe
*/


#include <dex/dex_ir_collections.h>
#include "dex/leb128.h"
#include "code_item_accessors.h"
#include "dex_file_exception_helpers.h"


namespace dex_ir {

    static uint64_t ReadVarWidth(const uint8_t **data, uint8_t length, bool sign_extend) {
        uint64_t value = 0;
        for (uint32_t i = 0; i <= length; i++) {
            value |= static_cast<uint64_t>(*(*data)++) << (i * 8);
        }
        if (sign_extend) {
            int shift = (7 - length) * 8;
            return (static_cast<int64_t>(value) << shift) >> shift;
        }
        return value;
    }

    uint32_t GetDebugInfoStreamSize(const uint8_t *debug_info_stream) {
        const uint8_t *stream = debug_info_stream;
        DecodeUnsignedLeb128(&stream);  // line_start
        uint32_t parameters_size = DecodeUnsignedLeb128(&stream);
        for (uint32_t i = 0; i < parameters_size; ++i) {
            DecodeUnsignedLeb128P1(&stream);  // Parameter name.
        }

        for (;;) {
            uint8_t opcode = *stream++;
            switch (opcode) {
                case art::DexFile::DBG_END_SEQUENCE:
                    return stream - debug_info_stream;  // end of stream.
                case art::DexFile::DBG_ADVANCE_PC:
                    DecodeUnsignedLeb128(&stream);  // addr_diff
                    break;
                case art::DexFile::DBG_ADVANCE_LINE:
                    DecodeSignedLeb128(&stream);  // line_diff
                    break;
                case art::DexFile::DBG_START_LOCAL:
                    DecodeUnsignedLeb128(&stream);  // register_num
                    DecodeUnsignedLeb128P1(&stream);  // name_idx
                    DecodeUnsignedLeb128P1(&stream);  // type_idx
                    break;
                case art::DexFile::DBG_START_LOCAL_EXTENDED:
                    DecodeUnsignedLeb128(&stream);  // register_num
                    DecodeUnsignedLeb128P1(&stream);  // name_idx
                    DecodeUnsignedLeb128P1(&stream);  // type_idx
                    DecodeUnsignedLeb128P1(&stream);  // sig_idx
                    break;
                case art::DexFile::DBG_END_LOCAL:
                case art::DexFile::DBG_RESTART_LOCAL:
                    DecodeUnsignedLeb128(&stream);  // register_num
                    break;
                case art::DexFile::DBG_SET_PROLOGUE_END:
                case art::DexFile::DBG_SET_EPILOGUE_BEGIN:
                    break;
                case art::DexFile::DBG_SET_FILE: {
                    DecodeUnsignedLeb128P1(&stream);  // name_idx
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    /**
     * 获取x
     * @param collections  当前集合
     * @param dec_insn  Instruction
     * @param type_ids  需要修的集合
     * @param string_ids
     * @param method_ids
     * @param field_ids
     * @return
     */
    bool GetIdFromInstruction(Collections &collections,
                              const art::Instruction *dec_insn,
                              std::vector<TypeId *> *type_ids,
                              std::vector<StringId *> *string_ids,
                              std::vector<MethodId *> *method_ids,
                              std::vector<FieldId *> *field_ids) {
        // Determine index and width of the string.
        uint32_t index = 0;
        switch (art::Instruction::FormatOf(dec_insn->Opcode())) {
            // SOME NOT SUPPORTED:
            // case Instruction::k20bc:
            case art::Instruction::k21c:
            case art::Instruction::k35c:
                // case Instruction::k35ms:
            case art::Instruction::k3rc:
                // case Instruction::k3rms:
                // case Instruction::k35mi:
                // case Instruction::k3rmi:
            case art::Instruction::k45cc:
            case art::Instruction::k4rcc:
                index = dec_insn->VRegB();
                break;
            case art::Instruction::k31c:
                index = dec_insn->VRegB();
                break;
            case art::Instruction::k22c:
                // case art::Instruction::k22cs:
                index = dec_insn->VRegC();
                break;
            default:
                break;
        }  // switch

        // Determine index type, and add reference to the appropriate collection.
        switch (art::Instruction::IndexTypeOf(dec_insn->Opcode())) {
            case art::Instruction::kIndexTypeRef:
                if (index < collections.TypeIdsSize()) {
                    type_ids->push_back(collections.GetTypeId(index));
                    return true;
                }
                break;
            case art::Instruction::kIndexStringRef:
                if (index < collections.StringIdsSize()) {
                    string_ids->push_back(collections.GetStringId(index));
                    return true;
                }
                break;
            case art::Instruction::kIndexMethodRef:
            case art::Instruction::kIndexMethodAndProtoRef:
                if (index < collections.MethodIdsSize()) {
                    method_ids->push_back(collections.GetMethodId(index));
                    return true;
                }
                break;
            case art::Instruction::kIndexFieldRef:
                if (index < collections.FieldIdsSize()) {
                    field_ids->push_back(collections.GetFieldId(index));
                    return true;
                }
                break;
            case art::Instruction::kIndexUnknown:
            case art::Instruction::kIndexNone:
            case art::Instruction::kIndexVtableOffset:
            case art::Instruction::kIndexFieldOffset:
            default:
                break;
        }  // switch
        return false;
    }

/*
 * Get all the types, strings, methods, and fields referred to from bytecode.
 */
    bool GetIdsFromByteCode(Collections &collections,
                            const CodeItem *code,
                            std::vector<TypeId *> *type_ids,
                            std::vector<StringId *> *string_ids,
                            std::vector<MethodId *> *method_ids,
                            std::vector<FieldId *> *field_ids) {
        bool has_id = false;
        art::IterationRange<art::DexInstructionIterator> instructions = code->Instructions();
        art::SafeDexInstructionIterator it(instructions.begin(), instructions.end());
        for (; !it.IsErrorState() && it < instructions.end(); ++it) {
            // In case the instruction goes past the end of the code item, make sure to not process it.
            art::SafeDexInstructionIterator next = it;
            ++next;
            if (next.IsErrorState()) {
                break;
            }
            has_id |= GetIdFromInstruction(collections,
                                           &it.Inst(),
                                           type_ids,
                                           string_ids,
                                           method_ids,
                                           field_ids);
        }  // for
        return has_id;
    }

    EncodedValue *
    Collections::ReadEncodedValue(const art::DexFile &dex_file, const uint8_t **data) {
        const uint8_t encoded_value = *(*data)++;
        const uint8_t type = encoded_value & 0x1f;
        EncodedValue *item = new EncodedValue(type);
        ReadEncodedValue(dex_file, data, type, encoded_value >> 5, item);
        return item;
    }

    EncodedValue *Collections::ReadEncodedValue(const art::DexFile &dex_file,
                                                const uint8_t **data,
                                                uint8_t type,
                                                uint8_t length) {
        EncodedValue *item = new EncodedValue(type);
        ReadEncodedValue(dex_file, data, type, length, item);
        return item;
    }

    void Collections::ReadEncodedValue(const art::DexFile &dex_file,
                                       const uint8_t **data,
                                       uint8_t type,
                                       uint8_t length,
                                       EncodedValue *item) {
        switch (type) {
            case art::DexFile::kDexAnnotationByte:
                item->SetByte(static_cast<int8_t>(ReadVarWidth(data, length, false)));
                break;
            case art::DexFile::kDexAnnotationShort:
                item->SetShort(static_cast<int16_t>(ReadVarWidth(data, length, true)));
                break;
            case art::DexFile::kDexAnnotationChar:
                item->SetChar(static_cast<uint16_t>(ReadVarWidth(data, length, false)));
                break;
            case art::DexFile::kDexAnnotationInt:
                item->SetInt(static_cast<int32_t>(ReadVarWidth(data, length, true)));
                break;
            case art::DexFile::kDexAnnotationLong:
                item->SetLong(static_cast<int64_t>(ReadVarWidth(data, length, true)));
                break;
            case art::DexFile::kDexAnnotationFloat: {
                // Fill on right.
                union {
                    float f;
                    uint32_t data;
                } conv;
                conv.data = static_cast<uint32_t>(ReadVarWidth(data, length, false))
                        << (3 - length) * 8;
                item->SetFloat(conv.f);
                break;
            }
            case art::DexFile::kDexAnnotationDouble: {
                // Fill on right.
                union {
                    double d;
                    uint64_t data;
                } conv;
                conv.data = ReadVarWidth(data, length, false) << (7 - length) * 8;
                item->SetDouble(conv.d);
                break;
            }
            case art::DexFile::kDexAnnotationMethodType: {
                const uint32_t proto_index = static_cast<uint32_t>(ReadVarWidth(data, length,
                                                                                false));
                item->SetProtoId(GetProtoId(proto_index));
                break;
            }
            case art::DexFile::kDexAnnotationMethodHandle: {
                const uint32_t method_handle_index = static_cast<uint32_t>(ReadVarWidth(data,
                                                                                        length,
                                                                                        false));
                item->SetMethodHandle(GetMethodHandle(method_handle_index));
                break;
            }
            case art::DexFile::kDexAnnotationString: {
                const uint32_t string_index = static_cast<uint32_t>(ReadVarWidth(data, length,
                                                                                 false));
                item->SetStringId(GetStringId(string_index));
                break;
            }
            case art::DexFile::kDexAnnotationType: {
                const uint32_t string_index = static_cast<uint32_t>(ReadVarWidth(data, length,
                                                                                 false));
                item->SetTypeId(GetTypeId(string_index));
                break;
            }
            case art::DexFile::kDexAnnotationField:
            case art::DexFile::kDexAnnotationEnum: {
                const uint32_t field_index = static_cast<uint32_t>(ReadVarWidth(data, length,
                                                                                false));
                item->SetFieldId(GetFieldId(field_index));
                break;
            }
            case art::DexFile::kDexAnnotationMethod: {
                const uint32_t method_index = static_cast<uint32_t>(ReadVarWidth(data, length,
                                                                                 false));
                item->SetMethodId(GetMethodId(method_index));
                break;
            }
            case art::DexFile::kDexAnnotationArray: {
                EncodedValueVector *values = new EncodedValueVector();
                const uint32_t offset = *data - dex_file.DataBegin();
                const uint32_t size = DecodeUnsignedLeb128(data);
                // Decode all elements.
                for (uint32_t i = 0; i < size; i++) {
                    values->push_back(
                            std::unique_ptr<EncodedValue>(ReadEncodedValue(dex_file, data)));
                }
                EncodedArrayItem *array_item = new EncodedArrayItem(values);
                if (eagerly_assign_offsets_) {
                    array_item->SetOffset(offset);
                }
                item->SetEncodedArray(array_item);
                break;
            }
            case art::DexFile::kDexAnnotationAnnotation: {
                AnnotationElementVector *elements = new AnnotationElementVector();
                const uint32_t type_idx = DecodeUnsignedLeb128(data);
                const uint32_t size = DecodeUnsignedLeb128(data);
                // Decode all name=value pairs.
                for (uint32_t i = 0; i < size; i++) {
                    const uint32_t name_index = DecodeUnsignedLeb128(data);
                    elements->push_back(std::unique_ptr<AnnotationElement>(
                            new AnnotationElement(GetStringId(name_index),
                                                  ReadEncodedValue(dex_file, data))));
                }
                item->SetEncodedAnnotation(new EncodedAnnotation(GetTypeId(type_idx), elements));
                break;
            }
            case art::DexFile::kDexAnnotationNull:
                break;
            case art::DexFile::kDexAnnotationBoolean:
                item->SetBoolean(length != 0);
                break;
            default:
                break;
        }
    }

    void Collections::CreateStringId(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::StringId &disk_string_id = dex_file.GetStringId(
                art::StringIndex(i));
        StringData *string_data = new StringData(dex_file.GetStringData(disk_string_id));
        AddItem(string_datas_map_, string_datas_, string_data, disk_string_id.string_data_off_);

        StringId *string_id = new StringId(string_data);
        AddIndexedItem(string_ids_, string_id, StringIdsOffset() + i * StringId::ItemSize(), i);
    }

    void Collections::CreateTypeId(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::TypeId &disk_type_id = dex_file.GetTypeId(art::TypeIndex(i));
        TypeId *type_id = new TypeId(GetStringId(disk_type_id.descriptor_idx_.index_));
        AddIndexedItem(type_ids_, type_id, TypeIdsOffset() + i * TypeId::ItemSize(), i);
    }

    void Collections::CreateProtoId(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::ProtoId &disk_proto_id = dex_file.GetProtoId(i);
        const art::DexFile::TypeList *type_list = dex_file.GetProtoParameters(disk_proto_id);
        TypeList *parameter_type_list = CreateTypeList(type_list,
                                                       disk_proto_id.parameters_off_);

        ProtoId *proto_id = new ProtoId(GetStringId(disk_proto_id.shorty_idx_.index_),
                                        GetTypeId(disk_proto_id.return_type_idx_.index_),
                                        parameter_type_list);
        AddIndexedItem(proto_ids_, proto_id, ProtoIdsOffset() + i * ProtoId::ItemSize(), i);
    }

    void Collections::CreateFieldId(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::FieldId &disk_field_id = dex_file.GetFieldId(i);
        FieldId *field_id = new FieldId(GetTypeId(disk_field_id.class_idx_.index_),
                                        GetTypeId(disk_field_id.type_idx_.index_),
                                        GetStringId(disk_field_id.name_idx_.index_));
        AddIndexedItem(field_ids_, field_id, FieldIdsOffset() + i * FieldId::ItemSize(), i);
    }

    void Collections::CreateMethodId(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::MethodId &disk_method_id = dex_file.GetMethodId(i);
        MethodId *method_id = new MethodId(GetTypeId(disk_method_id.class_idx_.index_),
                                           GetProtoId(disk_method_id.proto_idx_),
                                           GetStringId(disk_method_id.name_idx_.index_));
        AddIndexedItem(method_ids_, method_id, MethodIdsOffset() + i * MethodId::ItemSize(), i);
    }

    void Collections::CreateClassDef(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::ClassDef &disk_class_def = dex_file.GetClassDef(i);
        const TypeId *class_type = GetTypeId(disk_class_def.class_idx_.index_);
        uint32_t access_flags = disk_class_def.access_flags_;
        const TypeId *superclass = GetTypeIdOrNullPtr(disk_class_def.superclass_idx_.index_);

        const art::DexFile::TypeList *type_list = dex_file.GetInterfacesList(disk_class_def);
        TypeList *interfaces_type_list = CreateTypeList(type_list,
                                                        disk_class_def.interfaces_off_);

        const StringId *source_file = GetStringIdOrNullPtr(
                disk_class_def.source_file_idx_.index_);
        // Annotations.
        AnnotationsDirectoryItem *annotations = nullptr;
        const art::DexFile::AnnotationsDirectoryItem *disk_annotations_directory_item =
                dex_file.GetAnnotationsDirectory(disk_class_def);
        if (disk_annotations_directory_item != nullptr) {
            annotations = CreateAnnotationsDirectoryItem(
                    dex_file, disk_annotations_directory_item, disk_class_def.annotations_off_);
        }
        // Static field initializers.
        const uint8_t *static_data = dex_file.GetEncodedStaticFieldValuesArray(disk_class_def);
        EncodedArrayItem *static_values =
                CreateEncodedArrayItem(dex_file, static_data,
                                       disk_class_def.static_values_off_);
        ClassData *class_data = CreateClassData(
                dex_file, dex_file.GetClassData(disk_class_def),
                disk_class_def.class_data_off_);
        ClassDef *class_def = new ClassDef(class_type, access_flags, superclass,
                                           interfaces_type_list,
                                           source_file, annotations, static_values, class_data);
        if (class_data != nullptr) {
            class_data->SetClassDef(class_def);
        }
        AddIndexedItem(class_defs_, class_def, ClassDefsOffset() + i * ClassDef::ItemSize(), i);
    }

    TypeList *
    Collections::CreateTypeList(const art::DexFile::TypeList *dex_type_list, uint32_t offset) {
        if (dex_type_list == nullptr) {
            return nullptr;
        }
        TypeList *type_list = type_lists_map_.GetExistingObject(offset);
        if (type_list == nullptr) {
            TypeIdVector *type_vector = new TypeIdVector();
            uint32_t size = dex_type_list->Size();
            for (uint32_t index = 0; index < size; ++index) {
                type_vector->push_back(
                        GetTypeId(dex_type_list->GetTypeItem(index).type_idx_.index_));
            }
            type_list = new TypeList(type_vector);
            AddItem(type_lists_map_, type_lists_, type_list, offset);
        }
        return type_list;
    }

    EncodedArrayItem *Collections::CreateEncodedArrayItem(const art::DexFile &dex_file,
                                                          const uint8_t *static_data,
                                                          uint32_t offset) {
        if (static_data == nullptr) {
            return nullptr;
        }
        EncodedArrayItem *encoded_array_item = encoded_array_items_map_.GetExistingObject(offset);
        if (encoded_array_item == nullptr) {
            uint32_t size = DecodeUnsignedLeb128(&static_data);
            EncodedValueVector *values = new EncodedValueVector();
            for (uint32_t i = 0; i < size; ++i) {
                values->push_back(
                        std::unique_ptr<EncodedValue>(ReadEncodedValue(dex_file, &static_data)));
            }
            // TODO: Calculate the size of the encoded array.
            encoded_array_item = new EncodedArrayItem(values);
            AddItem(encoded_array_items_map_, encoded_array_items_, encoded_array_item, offset);
        }
        return encoded_array_item;
    }

    void Collections::AddAnnotationsFromMapListSection(const art::DexFile &dex_file,
                                                       uint32_t start_offset,
                                                       uint32_t count) {
        uint32_t current_offset = start_offset;
        for (size_t i = 0; i < count; ++i) {
            // Annotation that we didn't process already, add it to the set.
            const art::DexFile::AnnotationItem *annotation = dex_file.GetAnnotationItemAtOffset(
                    current_offset);
            AnnotationItem *annotation_item = CreateAnnotationItem(dex_file, annotation);
            DCHECK(annotation_item != nullptr);
            current_offset += annotation_item->GetSize();
        }
    }

    AnnotationItem *Collections::CreateAnnotationItem(const art::DexFile &dex_file,
                                                      const art::DexFile::AnnotationItem *annotation) {
        const uint8_t *const start_data = reinterpret_cast<const uint8_t *>(annotation);
        const uint32_t offset = start_data - dex_file.DataBegin();
        AnnotationItem *annotation_item = annotation_items_map_.GetExistingObject(offset);
        if (annotation_item == nullptr) {
            uint8_t visibility = annotation->visibility_;
            const uint8_t *annotation_data = annotation->annotation_;
            EncodedValue *encoded_value(
                    ReadEncodedValue(dex_file, &annotation_data,
                                     art::DexFile::kDexAnnotationAnnotation, 0));
            annotation_item = new AnnotationItem(visibility,
                                                 encoded_value->ReleaseEncodedAnnotation());
            annotation_item->SetSize(annotation_data - start_data);
            AddItem(annotation_items_map_, annotation_items_, annotation_item, offset);
        }
        return annotation_item;
    }


    AnnotationSetItem *Collections::CreateAnnotationSetItem(const art::DexFile &dex_file,
                                                            const art::DexFile::AnnotationSetItem *disk_annotations_item,
                                                            uint32_t offset) {
        if (disk_annotations_item == nullptr ||
            (disk_annotations_item->size_ == 0 && offset == 0)) {
            return nullptr;
        }
        AnnotationSetItem *annotation_set_item = annotation_set_items_map_.GetExistingObject(
                offset);
        if (annotation_set_item == nullptr) {
            std::vector<AnnotationItem *> *items = new std::vector<AnnotationItem *>();
            for (uint32_t i = 0; i < disk_annotations_item->size_; ++i) {
                const art::DexFile::AnnotationItem *annotation =
                        dex_file.GetAnnotationItem(disk_annotations_item, i);
                if (annotation == nullptr) {
                    continue;
                }
                AnnotationItem *annotation_item = CreateAnnotationItem(dex_file, annotation);
                items->push_back(annotation_item);
            }
            annotation_set_item = new AnnotationSetItem(items);
            AddItem(annotation_set_items_map_, annotation_set_items_, annotation_set_item,
                    offset);
        }
        return annotation_set_item;
    }

    AnnotationsDirectoryItem *
    Collections::CreateAnnotationsDirectoryItem(const art::DexFile &dex_file,
                                                const art::DexFile::AnnotationsDirectoryItem *disk_annotations_item,
                                                uint32_t offset) {
        AnnotationsDirectoryItem *annotations_directory_item =
                annotations_directory_items_map_.GetExistingObject(offset);
        if (annotations_directory_item != nullptr) {
            return annotations_directory_item;
        }
        const art::DexFile::AnnotationSetItem *class_set_item =
                dex_file.GetClassAnnotationSet(disk_annotations_item);
        AnnotationSetItem *class_annotation = nullptr;
        if (class_set_item != nullptr) {
            uint32_t item_offset = disk_annotations_item->class_annotations_off_;
            class_annotation = CreateAnnotationSetItem(dex_file, class_set_item, item_offset);
        }
        const art::DexFile::FieldAnnotationsItem *fields =
                dex_file.GetFieldAnnotations(disk_annotations_item);
        FieldAnnotationVector *field_annotations = nullptr;
        if (fields != nullptr) {
            field_annotations = new FieldAnnotationVector();
            for (uint32_t i = 0; i < disk_annotations_item->fields_size_; ++i) {
                FieldId *field_id = GetFieldId(fields[i].field_idx_);
                const art::DexFile::AnnotationSetItem *field_set_item =
                        dex_file.GetFieldAnnotationSetItem(fields[i]);
                uint32_t annotation_set_offset = fields[i].annotations_off_;
                AnnotationSetItem *annotation_set_item =
                        CreateAnnotationSetItem(dex_file, field_set_item, annotation_set_offset);
                field_annotations->push_back(std::unique_ptr<FieldAnnotation>(
                        new FieldAnnotation(field_id, annotation_set_item)));
            }
        }
        const art::DexFile::MethodAnnotationsItem *methods =
                dex_file.GetMethodAnnotations(disk_annotations_item);
        MethodAnnotationVector *method_annotations = nullptr;
        if (methods != nullptr) {
            method_annotations = new MethodAnnotationVector();
            for (uint32_t i = 0; i < disk_annotations_item->methods_size_; ++i) {
                MethodId *method_id = GetMethodId(methods[i].method_idx_);
                const art::DexFile::AnnotationSetItem *method_set_item =
                        dex_file.GetMethodAnnotationSetItem(methods[i]);
                uint32_t annotation_set_offset = methods[i].annotations_off_;
                AnnotationSetItem *annotation_set_item =
                        CreateAnnotationSetItem(dex_file, method_set_item, annotation_set_offset);
                method_annotations->push_back(std::unique_ptr<MethodAnnotation>(
                        new MethodAnnotation(method_id, annotation_set_item)));
            }
        }
        const art::DexFile::ParameterAnnotationsItem *parameters =
                dex_file.GetParameterAnnotations(disk_annotations_item);
        ParameterAnnotationVector *parameter_annotations = nullptr;
        if (parameters != nullptr) {
            parameter_annotations = new ParameterAnnotationVector();
            for (uint32_t i = 0; i < disk_annotations_item->parameters_size_; ++i) {
                MethodId *method_id = GetMethodId(parameters[i].method_idx_);
                const art::DexFile::AnnotationSetRefList *list =
                        dex_file.GetParameterAnnotationSetRefList(&parameters[i]);
                parameter_annotations->push_back(std::unique_ptr<ParameterAnnotation>(
                        GenerateParameterAnnotation(dex_file, method_id, list,
                                                    parameters[i].annotations_off_)));
            }
        }
        // TODO: Calculate the size of the annotations directory.
        annotations_directory_item = new AnnotationsDirectoryItem(
                class_annotation, field_annotations, method_annotations, parameter_annotations);
        AddItem(annotations_directory_items_map_,
                annotations_directory_items_,
                annotations_directory_item,
                offset);
        return annotations_directory_item;
    }

    ParameterAnnotation *Collections::GenerateParameterAnnotation(
            const art::DexFile &dex_file, MethodId *method_id,
            const art::DexFile::AnnotationSetRefList *annotation_set_ref_list, uint32_t offset) {
        AnnotationSetRefList *set_ref_list = annotation_set_ref_lists_map_.GetExistingObject(
                offset);
        if (set_ref_list == nullptr) {
            std::vector<AnnotationSetItem *> *annotations = new std::vector<AnnotationSetItem *>();
            for (uint32_t i = 0; i < annotation_set_ref_list->size_; ++i) {
                const art::DexFile::AnnotationSetItem *annotation_set_item =
                        dex_file.GetSetRefItemItem(&annotation_set_ref_list->list_[i]);
                uint32_t set_offset = annotation_set_ref_list->list_[i].annotations_off_;
                annotations->push_back(
                        CreateAnnotationSetItem(dex_file, annotation_set_item, set_offset));
            }
            set_ref_list = new AnnotationSetRefList(annotations);
            AddItem(annotation_set_ref_lists_map_, annotation_set_ref_lists_, set_ref_list,
                    offset);
        }
        return new ParameterAnnotation(method_id, set_ref_list);
    }


    CodeItem *Collections::DedupeOrCreateCodeItem(const art::DexFile &dex_file,
                                                  const art::DexFile::CodeItem *disk_code_item,
                                                  uint32_t offset,
                                                  uint32_t dex_method_index) {
        if (disk_code_item == nullptr) {
            return nullptr;
        }

        art::CodeItemDebugInfoAccessor accessor(dex_file, disk_code_item, dex_method_index);
        uint32_t debug_info_offset = accessor.DebugInfoOffset();

        if (debug_info_offset < dex_file.GetHeader().file_size_) {
            debug_info_offset = 0;
        }

        // Create the offsets pair and dedupe based on it.
        std::pair<uint32_t, uint32_t> offsets_pair(offset, debug_info_offset);
        auto existing = code_items_map_.find(offsets_pair);
        if (existing != code_items_map_.end()) {
            return existing->second;
        }

        const uint8_t *debug_info_stream = dex_file.GetDebugInfoStream(debug_info_offset);
        DebugInfoItem *debug_info = nullptr;
        if (debug_info_stream != nullptr) {
            debug_info = debug_info_items_map_.GetExistingObject(debug_info_offset);
            if (debug_info == nullptr) {
                uint32_t debug_info_size = GetDebugInfoStreamSize(debug_info_stream);
                uint8_t *debug_info_buffer = new uint8_t[debug_info_size];
                memcpy(debug_info_buffer, debug_info_stream, debug_info_size);
                debug_info = new DebugInfoItem(debug_info_size, debug_info_buffer);
                AddItem(debug_info_items_map_, debug_info_items_, debug_info, debug_info_offset);
            }
        }

        uint32_t insns_size = accessor.InsnsSizeInCodeUnits();
        uint16_t *insns = new uint16_t[insns_size];
        memcpy(insns, accessor.Insns(), insns_size * sizeof(uint16_t));

        TryItemVector *tries = nullptr;
        CatchHandlerVector *handler_list = nullptr;
        if (accessor.TriesSize() > 0) {
            tries = new TryItemVector();
            handler_list = new CatchHandlerVector();
            for (const art::DexFile::TryItem &disk_try_item : accessor.TryItems()) {
                uint32_t start_addr = disk_try_item.start_addr_;
                uint16_t insn_count = disk_try_item.insn_count_;
                uint16_t handler_off = disk_try_item.handler_off_;
                const CatchHandler *handlers = nullptr;
                for (std::unique_ptr<const CatchHandler> &existing_handlers : *handler_list) {
                    if (handler_off == existing_handlers->GetListOffset()) {
                        handlers = existing_handlers.get();
                        break;
                    }
                }
                if (handlers == nullptr) {
                    bool catch_all = false;
                    TypeAddrPairVector *addr_pairs = new TypeAddrPairVector();
                    for (art::CatchHandlerIterator it(accessor,
                                                         disk_try_item); it.HasNext(); it.Next()) {
                        const art::TypeIndex type_index = it.GetHandlerTypeIndex();
                        const TypeId *type_id = GetTypeIdOrNullPtr(type_index.index_);
                        catch_all |= type_id == nullptr;
                        addr_pairs->push_back(std::unique_ptr<const TypeAddrPair>(
                                new TypeAddrPair(type_id, it.GetHandlerAddress())));
                    }
                    handlers = new CatchHandler(catch_all, handler_off, addr_pairs);
                    handler_list->push_back(std::unique_ptr<const CatchHandler>(handlers));
                }
                TryItem *try_item = new TryItem(start_addr, insn_count, handlers);
                tries->push_back(std::unique_ptr<const TryItem>(try_item));
            }
            // Manually walk catch handlers list and add any missing handlers unreferenced by try items.
            const uint8_t *handlers_base = accessor.GetCatchHandlerData();
            const uint8_t *handlers_data = handlers_base;
            uint32_t handlers_size = DecodeUnsignedLeb128(&handlers_data);
            while (handlers_size > handler_list->size()) {
                bool already_added = false;
                uint16_t handler_off = handlers_data - handlers_base;
                for (std::unique_ptr<const CatchHandler> &existing_handlers : *handler_list) {
                    if (handler_off == existing_handlers->GetListOffset()) {
                        already_added = true;
                        break;
                    }
                }
                int32_t size = DecodeSignedLeb128(&handlers_data);
                bool has_catch_all = size <= 0;
                if (has_catch_all) {
                    size = -size;
                }
                if (already_added) {
                    for (int32_t i = 0; i < size; i++) {
                        DecodeUnsignedLeb128(&handlers_data);
                        DecodeUnsignedLeb128(&handlers_data);
                    }
                    if (has_catch_all) {
                        DecodeUnsignedLeb128(&handlers_data);
                    }
                    continue;
                }
                TypeAddrPairVector *addr_pairs = new TypeAddrPairVector();
                for (int32_t i = 0; i < size; i++) {
                    const TypeId *type_id = GetTypeIdOrNullPtr(
                            DecodeUnsignedLeb128(&handlers_data));
                    uint32_t addr = DecodeUnsignedLeb128(&handlers_data);
                    addr_pairs->push_back(
                            std::unique_ptr<const TypeAddrPair>(new TypeAddrPair(type_id, addr)));
                }
                if (has_catch_all) {
                    uint32_t addr = DecodeUnsignedLeb128(&handlers_data);
                    addr_pairs->push_back(
                            std::unique_ptr<const TypeAddrPair>(new TypeAddrPair(nullptr, addr)));
                }
                const CatchHandler *handler = new CatchHandler(has_catch_all, handler_off,
                                                               addr_pairs);
                handler_list->push_back(std::unique_ptr<const CatchHandler>(handler));
            }
        }

        uint32_t size = dex_file.GetCodeItemSize(*disk_code_item);
        CodeItem *code_item = new CodeItem(accessor.RegistersSize(),
                                           accessor.InsSize(),
                                           accessor.OutsSize(),
                                           debug_info,
                                           insns_size,
                                           insns,
                                           tries,
                                           handler_list);
        code_item->SetSize(size);
        if (accessor.DebugInfoOffset() != 0 && debug_info == nullptr) {
            code_item->SetDebugInfoOffset(accessor.DebugInfoOffset());
        }

        // Add the code item to the map.
        DCHECK(!code_item->OffsetAssigned());
        if (eagerly_assign_offsets_) {
            code_item->SetOffset(offset);
        }
        code_items_map_.emplace(offsets_pair, code_item);
        code_items_.AddItem(code_item);

        // Add "fixup" references to types, strings, methods, and fields.
        // This is temporary, as we will probably want more detailed parsing of the
        // instructions here.
        /* std::vector<TypeId *> type_ids;
         std::vector<StringId *> string_ids;
         std::vector<MethodId *> method_ids;
         std::vector<FieldId *> field_ids;
         if (GetIdsFromByteCode(*this,
                                code_item,
                 *//*out*//* &type_ids,
                *//*out*//* &string_ids,
                *//*out*//* &method_ids,
                *//*out*//* &field_ids)) {
            CodeFixups *fixups = new CodeFixups(std::move(type_ids),
                                                std::move(string_ids),
                                                std::move(method_ids),
                                                std::move(field_ids));
            code_item->SetCodeFixups(fixups);
        }
*/
        return code_item;
    }

    MethodItem *Collections::GenerateMethodItem(const art::DexFile &dex_file,
                                                art::ClassDataItemIterator &cdii) {

        uint32_t raw_method_index = cdii.GetMemberIndex();
        MethodId *method_id = GetMethodId(raw_method_index);
        uint32_t access_flags = cdii.GetRawMemberAccessFlags();
        const art::DexFile::CodeItem *disk_code_item = cdii.GetMethodCodeItem();
        // Temporary hack to prevent incorrectly deduping code items if they have the same offset since
        // they may have different debug info streams.
        CodeItem *code_item = DedupeOrCreateCodeItem(dex_file,
                                                     disk_code_item,
                                                     cdii.GetMethodCodeItemOffset(),
                                                     cdii.GetMemberIndex());
        MethodItem *methodItem = new MethodItem(access_flags, method_id, code_item, raw_method_index);
        if (code_item != nullptr) {
            code_item->SetMethodItem(methodItem);
        }
        if (methodItem != nullptr) {
            method_items_.insert(std::make_pair(raw_method_index, methodItem));
        }
        return methodItem;
    }

    ClassData *
    Collections::CreateClassData(const art::DexFile &dex_file, const uint8_t *encoded_data, uint32_t offset) {
        // Read the fields and methods defined by the class, resolving the circular reference from those
        // to classes by setting class at the same time.
        ClassData *class_data = class_datas_map_.GetExistingObject(offset);
        if (class_data == nullptr && encoded_data != nullptr) {
            art::ClassDataItemIterator cdii(dex_file, encoded_data);
            // Static fields.
            FieldItemVector *static_fields = new FieldItemVector();
            for (; cdii.HasNextStaticField(); cdii.Next()) {
                FieldId *field_item = GetFieldId(cdii.GetMemberIndex());
                uint32_t access_flags = cdii.GetRawMemberAccessFlags();
                static_fields->push_back(std::unique_ptr<FieldItem>
                                                 (new FieldItem(access_flags, field_item)));
            }
            // Instance fields.
            FieldItemVector *instance_fields = new FieldItemVector();
            for (; cdii.HasNextInstanceField(); cdii.Next()) {
                FieldId *field_item = GetFieldId(cdii.GetMemberIndex());
                uint32_t access_flags = cdii.GetRawMemberAccessFlags();
                instance_fields->push_back(std::unique_ptr<FieldItem>
                                                   (new FieldItem(access_flags, field_item)));
            }
            // Direct methods.
            MethodItemVector *direct_methods = new MethodItemVector();
            for (; cdii.HasNextDirectMethod(); cdii.Next()) {
                direct_methods->push_back(std::unique_ptr<MethodItem>
                                                  (GenerateMethodItem(dex_file, cdii)));
            }
            // Virtual methods.
            MethodItemVector *virtual_methods = new MethodItemVector();
            for (; cdii.HasNextVirtualMethod(); cdii.Next()) {
                virtual_methods->push_back(std::unique_ptr<MethodItem>
                                                   (GenerateMethodItem(dex_file, cdii)));
            }
            class_data = new ClassData(static_fields, instance_fields, direct_methods,
                                       virtual_methods);
            class_data->SetSize(cdii.EndDataPointer() - encoded_data);

            for (int i = 0; i < static_fields->size(); ++i) {
                static_fields->at(i)->SetClassData(class_data);
            }

            for (int i = 0; i < instance_fields->size(); ++i) {
                instance_fields->at(i)->SetClassData(class_data);
            }

            for (int i = 0; i < direct_methods->size(); ++i) {
                direct_methods->at(i)->SetClassData(class_data);
            }
            for (int i = 0; i < virtual_methods->size(); ++i) {
                virtual_methods->at(i)->SetClassData(class_data);
            }
            AddItem(class_datas_map_, class_datas_, class_data, offset);
        }
        return class_data;
    }

    void Collections::CreateCallSitesAndMethodHandles(const art::DexFile &dex_file) {
        // Iterate through the map list and set the offset of the CallSiteIds and MethodHandleItems.
        const art::DexFile::MapList *map = dex_file.GetMapList();
        for (uint32_t i = 0; i < map->size_; ++i) {
            const art::DexFile::MapItem *item = map->list_ + i;
            switch (item->type_) {
                case art::DexFile::kDexTypeCallSiteIdItem:
                    SetCallSiteIdsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeMethodHandleItem:
                    SetMethodHandleItemsOffset(item->offset_);
                    break;
                default:
                    break;
            }
        }
        // Populate MethodHandleItems first (CallSiteIds may depend on them).
        for (uint32_t i = 0; i < dex_file.NumMethodHandles(); i++) {
            CreateMethodHandleItem(dex_file, i);
        }
        // Populate CallSiteIds.
        for (uint32_t i = 0; i < dex_file.NumCallSiteIds(); i++) {
            CreateCallSiteId(dex_file, i);
        }
    }

    void Collections::CreateCallSiteId(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::CallSiteIdItem &disk_call_site_id = dex_file.GetCallSiteId(i);
        const uint8_t *disk_call_item_ptr = dex_file.DataBegin() + disk_call_site_id.data_off_;
        EncodedArrayItem *call_site_item =
                CreateEncodedArrayItem(dex_file, disk_call_item_ptr,
                                       disk_call_site_id.data_off_);

        CallSiteId *call_site_id = new CallSiteId(call_site_item);
        AddIndexedItem(call_site_ids_, call_site_id,
                       CallSiteIdsOffset() + i * CallSiteId::ItemSize(), i);
    }

    void Collections::CreateMethodHandleItem(const art::DexFile &dex_file, uint32_t i) {
        const art::DexFile::MethodHandleItem &disk_method_handle = dex_file.GetMethodHandle(i);
        uint16_t index = disk_method_handle.field_or_method_idx_;
        art::DexFile::MethodHandleType type =
                static_cast<art::DexFile::MethodHandleType>(disk_method_handle.method_handle_type_);
        bool is_invoke = type == art::DexFile::MethodHandleType::kInvokeStatic ||
                         type == art::DexFile::MethodHandleType::kInvokeInstance ||
                         type == art::DexFile::MethodHandleType::kInvokeConstructor ||
                         type == art::DexFile::MethodHandleType::kInvokeDirect ||
                         type == art::DexFile::MethodHandleType::kInvokeInterface;
        static_assert(
                art::DexFile::MethodHandleType::kLast ==
                art::DexFile::MethodHandleType::kInvokeInterface,
                "Unexpected method handle types.");
        IndexedItem *field_or_method_id;
        if (is_invoke) {
            field_or_method_id = GetMethodId(index);
        } else {
            field_or_method_id = GetFieldId(index);
        }
        MethodHandleItem *method_handle = new MethodHandleItem(type, field_or_method_id);
        AddIndexedItem(method_handle_items_,
                       method_handle,
                       MethodHandleItemsOffset() + i * MethodHandleItem::ItemSize(),
                       i);
    }

    void Collections::SortVectorsByMapOrder() {
        string_datas_.SortByMapOrder(string_datas_map_.Collection());
        type_lists_.SortByMapOrder(type_lists_map_.Collection());
        encoded_array_items_.SortByMapOrder(encoded_array_items_map_.Collection());
        annotation_items_.SortByMapOrder(annotation_items_map_.Collection());
        annotation_set_items_.SortByMapOrder(annotation_set_items_map_.Collection());
        annotation_set_ref_lists_.SortByMapOrder(annotation_set_ref_lists_map_.Collection());
        annotations_directory_items_.SortByMapOrder(
                annotations_directory_items_map_.Collection());
        debug_info_items_.SortByMapOrder(debug_info_items_map_.Collection());
        code_items_.SortByMapOrder(code_items_map_);
        class_datas_.SortByMapOrder(class_datas_map_.Collection());
    }

    template<typename Type>
    void
    Collections::AddItem(CollectionMap<Type> &map, CollectionVector<Type> &vector, Type *item,
                         uint32_t offset) {
        DCHECK(!map.GetExistingObject(offset));
        DCHECK(!item->OffsetAssigned());
        if (eagerly_assign_offsets_) {
            item->SetOffset(offset);
        }
        map.AddItem(item, offset);
        vector.AddItem(item);
    }

    template<typename Type>
    void Collections::AddIndexedItem(IndexedCollectionVector<Type> &vector, Type *item,
                                     uint32_t offset, uint32_t index) {
        DCHECK(!item->OffsetAssigned());
        if (eagerly_assign_offsets_) {
            item->SetOffset(offset);
        }
        vector.AddIndexedItem(item, index);
    }

    bool Collections::fixCodeItem(CodeItem *item, uint8_t *code_data) {

        return false;
    }

    CodeItem *Collections::CreateCodeItem(const art::DexFile &dex_file, uint8_t *data, uint32_t off_in_dex,
                                          uint32_t dex_id_index) {
        if (data == nullptr) {
            return nullptr;
        }
        art::StandardDexFile::CodeItem *codeItem_dex = (art::StandardDexFile::CodeItem *) data;
        art::CodeItemDebugInfoAccessor accessor(dex_file, codeItem_dex, dex_id_index);
        const uint32_t debug_info_offset = accessor.DebugInfoOffset();
        const uint8_t *debug_info_stream = dex_file.GetDebugInfoStream(debug_info_offset);

        // Create the offsets pair and dedupe based on it.
        std::pair<uint32_t, uint32_t> offsets_pair(off_in_dex, debug_info_offset);
        auto existing = code_items_map_.find(offsets_pair);
        if (existing != code_items_map_.end()) {
            delete existing->second;
            code_items_map_.erase(existing);
        }
        DebugInfoItem *debug_info = nullptr;
        if (debug_info_stream != nullptr) {
            debug_info = debug_info_items_map_.GetExistingObject(debug_info_offset);
            if (debug_info == nullptr) {
                uint32_t debug_info_size = GetDebugInfoStreamSize(debug_info_stream);
                uint8_t *debug_info_buffer = new uint8_t[debug_info_size];
                memcpy(debug_info_buffer, debug_info_stream, debug_info_size);
                debug_info = new DebugInfoItem(debug_info_size, debug_info_buffer);
                AddItem(debug_info_items_map_, debug_info_items_, debug_info,
                        debug_info_offset);
            }
        }

        uint32_t insns_size = accessor.InsnsSizeInCodeUnits();
        uint16_t *insns = new uint16_t[insns_size];
        memcpy(insns, accessor.Insns(), insns_size * sizeof(uint16_t));

        TryItemVector *tries = nullptr;
        CatchHandlerVector *handler_list = nullptr;
        if (accessor.TriesSize() > 0) {
            tries = new TryItemVector();
            handler_list = new CatchHandlerVector();
            for (const art::DexFile::TryItem &disk_try_item : accessor.TryItems()) {
                uint32_t start_addr = disk_try_item.start_addr_;
                uint16_t insn_count = disk_try_item.insn_count_;
                uint16_t handler_off = disk_try_item.handler_off_;
                const CatchHandler *handlers = nullptr;
                for (std::unique_ptr<const CatchHandler> &existing_handlers : *handler_list) {
                    if (handler_off == existing_handlers->GetListOffset()) {
                        handlers = existing_handlers.get();
                        break;
                    }
                }
                if (handlers == nullptr) {
                    bool catch_all = false;
                    TypeAddrPairVector *addr_pairs = new TypeAddrPairVector();
                    for (art::CatchHandlerIterator it(accessor,
                                                         disk_try_item); it.HasNext(); it.Next()) {
                        const art::TypeIndex type_index = it.GetHandlerTypeIndex();
                        const TypeId *type_id = GetTypeIdOrNullPtr(type_index.index_);
                        catch_all |= type_id == nullptr;
                        addr_pairs->push_back(std::unique_ptr<const TypeAddrPair>(
                                new TypeAddrPair(type_id, it.GetHandlerAddress())));
                    }
                    handlers = new CatchHandler(catch_all, handler_off, addr_pairs);
                    handler_list->push_back(std::unique_ptr<const CatchHandler>(handlers));
                }
                TryItem *try_item = new TryItem(start_addr, insn_count, handlers);
                tries->push_back(std::unique_ptr<const TryItem>(try_item));
            }
            // Manually walk catch handlers list and add any missing handlers unreferenced by try items.
            const uint8_t *handlers_base = accessor.GetCatchHandlerData();
            const uint8_t *handlers_data = handlers_base;
            uint32_t handlers_size = DecodeUnsignedLeb128(&handlers_data);
            while (handlers_size > handler_list->size()) {
                bool already_added = false;
                uint16_t handler_off = handlers_data - handlers_base;
                for (std::unique_ptr<const CatchHandler> &existing_handlers : *handler_list) {
                    if (handler_off == existing_handlers->GetListOffset()) {
                        already_added = true;
                        break;
                    }
                }
                int32_t size = DecodeSignedLeb128(&handlers_data);
                bool has_catch_all = size <= 0;
                if (has_catch_all) {
                    size = -size;
                }
                if (already_added) {
                    for (int32_t i = 0; i < size; i++) {
                        DecodeUnsignedLeb128(&handlers_data);
                        DecodeUnsignedLeb128(&handlers_data);
                    }
                    if (has_catch_all) {
                        DecodeUnsignedLeb128(&handlers_data);
                    }
                    continue;
                }
                TypeAddrPairVector *addr_pairs = new TypeAddrPairVector();
                for (int32_t i = 0; i < size; i++) {
                    const TypeId *type_id = GetTypeIdOrNullPtr(
                            DecodeUnsignedLeb128(&handlers_data));
                    uint32_t addr = DecodeUnsignedLeb128(&handlers_data);
                    addr_pairs->push_back(
                            std::unique_ptr<const TypeAddrPair>(new TypeAddrPair(type_id, addr)));
                }
                if (has_catch_all) {
                    uint32_t addr = DecodeUnsignedLeb128(&handlers_data);
                    addr_pairs->push_back(
                            std::unique_ptr<const TypeAddrPair>(new TypeAddrPair(nullptr, addr)));
                }
                const CatchHandler *handler = new CatchHandler(has_catch_all, handler_off,
                                                               addr_pairs);
                handler_list->push_back(std::unique_ptr<const CatchHandler>(handler));
            }
        }


        uint32_t size = dex_file.GetCodeItemSize(*codeItem_dex);
        CodeItem *code_item = new CodeItem(accessor.RegistersSize(), accessor.InsSize(), accessor.OutsSize(),
                                           debug_info, insns_size, insns, tries, handler_list);


        code_item->SetSize(size);

        // Add the code item to the map.
        DCHECK(!code_item->OffsetAssigned());

        code_items_map_.emplace(offsets_pair, code_item);
        code_items_.AddItem(code_item);
        return code_item;
    }


    static uint32_t HeaderOffset(const dex_ir::Collections &collections) {
        return 0;
    }

    static uint32_t HeaderSize(const dex_ir::Collections &collections) {
        // Size is in elements, so there is only one header.
        return 1;
    }


}