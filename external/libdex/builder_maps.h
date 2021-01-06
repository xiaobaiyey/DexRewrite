//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_BUILDER_MAPS_H
#define BASE_BUILDER_MAPS_H

#include "collection_base.h"
#include "indexed_collection_vector.h"
#include "dex/class_accessor.h"
#include "collection_map.h"


namespace dex_ir {
    class Header;

    class BuilderMaps {
    public:
        BuilderMaps(Header *header, bool eagerly_assign_offsets)
                : header_(header), eagerly_assign_offsets_(eagerly_assign_offsets) {}

        void CreateStringId(const libdex::DexFile &dex_file, uint32_t i);

        void CreateTypeId(const libdex::DexFile &dex_file, uint32_t i);

        void CreateProtoId(const libdex::DexFile &dex_file, uint32_t i);

        void CreateFieldId(const libdex::DexFile &dex_file, uint32_t i);

        void CreateMethodId(const libdex::DexFile &dex_file, uint32_t i);

        void CreateClassDef(const libdex::DexFile &dex_file, uint32_t i);

        void CreateCallSiteId(const libdex::DexFile &dex_file, uint32_t i);

        void CreateMethodHandleItem(const libdex::DexFile &dex_file, uint32_t i);

        void CreateCallSitesAndMethodHandles(const libdex::DexFile &dex_file);

        TypeList *CreateTypeList(const libdex::dex::TypeList *type_list, uint32_t offset);

        EncodedArrayItem *CreateEncodedArrayItem(const libdex::DexFile &dex_file,
                                                 const uint8_t *static_data,
                                                 uint32_t offset);

        AnnotationItem *CreateAnnotationItem(const libdex::DexFile &dex_file,
                                             const libdex::dex::AnnotationItem *annotation);

        AnnotationSetItem *CreateAnnotationSetItem(const libdex::DexFile &dex_file,
                                                   const libdex::dex::AnnotationSetItem *disk_annotations_item,
                                                   uint32_t offset);

        AnnotationsDirectoryItem *CreateAnnotationsDirectoryItem(const libdex::DexFile &dex_file,
                                                                 const libdex::dex::AnnotationsDirectoryItem *disk_annotations_item,
                                                                 uint32_t offset);

        CodeItem *DedupeOrCreateCodeItem(const libdex::DexFile &dex_file,
                                         const libdex::dex::CodeItem *disk_code_item,
                                         uint32_t offset,
                                         uint32_t dex_method_index);



        ClassData *CreateClassData(const libdex::DexFile &dex_file, const libdex::dex::ClassDef &class_def);

        void AddAnnotationsFromMapListSection(const libdex::DexFile &dex_file,
                                              uint32_t start_offset,
                                              uint32_t count);

        void AddHiddenapiClassDataFromMapListSection(const libdex::DexFile &dex_file, uint32_t offset);

        //void CheckAndSetRemainingOffsets(const libdex::DexFile &dex_file, const libdex::Options &options);

        // Sort the vectors buy map order (same order that was used in the input file).
        void SortVectorsByMapOrder();

    private:
        bool GetIdsFromByteCode(const CodeItem *code,
                                std::vector<TypeId *> *type_ids,
                                std::vector<StringId *> *string_ids,
                                std::vector<MethodId *> *method_ids,
                                std::vector<FieldId *> *field_ids);

        bool GetIdFromInstruction(const libdex::Instruction *dec_insn,
                                  std::vector<TypeId *> *type_ids,
                                  std::vector<StringId *> *string_ids,
                                  std::vector<MethodId *> *method_ids,
                                  std::vector<FieldId *> *field_ids);

        EncodedValue *ReadEncodedValue(const libdex::DexFile &dex_file, const uint8_t **data);

        EncodedValue *ReadEncodedValue(const libdex::DexFile &dex_file,
                                       const uint8_t **data,
                                       uint8_t type,
                                       uint8_t length);

        void ReadEncodedValue(const libdex::DexFile &dex_file,
                              const uint8_t **data,
                              uint8_t type,
                              uint8_t length,
                              EncodedValue *item);

        std::unique_ptr<MethodItem>
        GenerateMethodItem(const libdex::DexFile &dex_file, const libdex::ClassAccessor::Method &method);

        ParameterAnnotation *GenerateParameterAnnotation(
                const libdex::DexFile &dex_file,
                MethodId *method_id,
                const libdex::dex::AnnotationSetRefList *annotation_set_ref_list,
                uint32_t offset);

        template<typename Type, class... Args>
        Type *CreateAndAddIndexedItem(IndexedCollectionVector<Type> &vector,
                                      uint32_t offset,
                                      uint32_t index,
                                      Args &&... args) {
            Type *item = vector.CreateAndAddIndexedItem(index, std::forward<Args>(args)...);
            DCHECK(!item->OffsetAssigned());
            if (eagerly_assign_offsets_) {
                item->SetOffset(offset);
            }
            return item;
        }

        Header *header_;
        // If we eagerly assign offsets during IR building or later after layout. Must be false if
        // changing the layout is enabled.
        bool eagerly_assign_offsets_;

        // Note: maps do not have ownership.
        CollectionMap<StringData> string_datas_map_;
        CollectionMap<TypeList> type_lists_map_;
        CollectionMap<EncodedArrayItem> encoded_array_items_map_;
        CollectionMap<AnnotationItem> annotation_items_map_;
        CollectionMap<AnnotationSetItem> annotation_set_items_map_;
        CollectionMap<AnnotationSetRefList> annotation_set_ref_lists_map_;
        CollectionMap<AnnotationsDirectoryItem> annotations_directory_items_map_;
        CollectionMap<DebugInfoItem> debug_info_items_map_;
        // Code item maps need to check both the debug info offset and debug info offset, do not use
        // CollectionMap.
        // First offset is the code item offset, second is the debug info offset.
        std::map<std::pair<uint32_t, uint32_t>, CodeItem *> code_items_map_;
        CollectionMap<ClassData> class_datas_map_;

        DISALLOW_COPY_AND_ASSIGN(BuilderMaps);
    };
}


#endif //BASE_BUILDER_MAPS_H
