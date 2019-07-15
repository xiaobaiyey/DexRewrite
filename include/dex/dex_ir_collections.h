/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:25 PM
* @ class describe
*/

#ifndef UNPACKER_COLLECTIONS_H
#define UNPACKER_COLLECTIONS_H

#include "dex_ir.h"
#include "dex_ir_collectionvector.h"
#include "dex_ir_collectionmap.h"
#include "dex_ir_indexed_collection_vector.h"
#include "dex_file.h"
#include "dex_ir_stringid.h"
#include "dex_ir_typeid.h"
#include "dex_ir_protoid.h"
#include "dex_ir_fieldid.h"
#include "dex_ir_methodid.h"
#include "dex_ir_classdef.h"
#include "dex_ir_callsiteid.h"
#include "dex_ir_methodhandle_item.h"
#include "dex_ir_stringdata.h"
#include "dex_ir_typelist.h"
#include "dex_ir_annotation.h"
#include "dex_ir_debuginfo_item.h"
#include "dex_ir_method_item.h"
#include "dex_ir_code_item.h"
#include "dex_ir_classdata.h"
#include <utility>

namespace dex_ir {
    class Collections {
    public:
        Collections() = default;

        CollectionVector<StringId>::Vector &StringIds() { return string_ids_.Collection(); }

        CollectionVector<TypeId>::Vector &TypeIds() { return type_ids_.Collection(); }

        CollectionVector<ProtoId>::Vector &ProtoIds() { return proto_ids_.Collection(); }

        CollectionVector<FieldId>::Vector &FieldIds() { return field_ids_.Collection(); }

        CollectionVector<MethodId>::Vector &MethodIds() { return method_ids_.Collection(); }

        CollectionVector<ClassDef>::Vector &ClassDefs() { return class_defs_.Collection(); }

        CollectionVector<CallSiteId>::Vector &
        CallSiteIds() { return call_site_ids_.Collection(); }

        CollectionVector<MethodHandleItem>::Vector &
        MethodHandleItems() { return method_handle_items_.Collection(); }

        CollectionVector<StringData>::Vector &
        StringDatas() { return string_datas_.Collection(); }

        CollectionVector<TypeList>::Vector &TypeLists() { return type_lists_.Collection(); }

        CollectionVector<EncodedArrayItem>::Vector &
        EncodedArrayItems() { return encoded_array_items_.Collection(); }

        CollectionVector<AnnotationItem>::Vector &
        AnnotationItems() { return annotation_items_.Collection(); }

        CollectionVector<AnnotationSetItem>::Vector &
        AnnotationSetItems() { return annotation_set_items_.Collection(); }

        CollectionVector<AnnotationSetRefList>::Vector &
        AnnotationSetRefLists() { return annotation_set_ref_lists_.Collection(); }

        CollectionVector<AnnotationsDirectoryItem>::Vector &
        AnnotationsDirectoryItems() { return annotations_directory_items_.Collection(); }

        CollectionVector<DebugInfoItem>::Vector &
        DebugInfoItems() { return debug_info_items_.Collection(); }

        CollectionVector<CodeItem>::Vector &CodeItems() { return code_items_.Collection(); }

        std::map<uint32_t, MethodItem *> &
        MethodItems() { return method_items_; }

        CollectionVector<ClassData>::Vector &ClassDatas() { return class_datas_.Collection(); }

        const CollectionVector<ClassDef>::Vector &
        ClassDefs() const { return class_defs_.Collection(); }

        void CreateStringId(const art::DexFile &dex_file, uint32_t i);

        void CreateTypeId(const art::DexFile &dex_file, uint32_t i);

        void CreateProtoId(const art::DexFile &dex_file, uint32_t i);

        void CreateFieldId(const art::DexFile &dex_file, uint32_t i);

        void CreateMethodId(const art::DexFile &dex_file, uint32_t i);

        void CreateClassDef(const art::DexFile &dex_file, uint32_t i);

        void CreateCallSiteId(const art::DexFile &dex_file, uint32_t i);

        void CreateMethodHandleItem(const art::DexFile &dex_file, uint32_t i);

        void CreateCallSitesAndMethodHandles(const art::DexFile &dex_file);

        TypeList *CreateTypeList(const art::DexFile::TypeList *type_list, uint32_t offset);

        EncodedArrayItem *CreateEncodedArrayItem(const art::DexFile &dex_file,
                                                 const uint8_t *static_data,
                                                 uint32_t offset);

        AnnotationItem *CreateAnnotationItem(const art::DexFile &dex_file,
                                             const art::DexFile::AnnotationItem *annotation);

        AnnotationSetItem *CreateAnnotationSetItem(const art::DexFile &dex_file,
                                                   const art::DexFile::AnnotationSetItem *disk_annotations_item,
                                                   uint32_t offset);

        AnnotationsDirectoryItem *CreateAnnotationsDirectoryItem(const art::DexFile &dex_file,
                                                                 const art::DexFile::AnnotationsDirectoryItem *disk_annotations_item,
                                                                 uint32_t offset);

        CodeItem *DedupeOrCreateCodeItem(const art::DexFile &dex_file,
                                         const art::DexFile::CodeItem *disk_code_item,
                                         uint32_t offset,
                                         uint32_t dex_method_index);


        CodeItem *
        CreateCodeItem(const art::DexFile &dex_file, uint8_t *data, uint32_t off_in_dex, uint32_t dex_id_index = 0);


        bool fixCodeItem(CodeItem *item, uint8_t *code_data);

        ClassData *
        CreateClassData(const art::DexFile &dex_file, const uint8_t *encoded_data, uint32_t offset);

        void AddAnnotationsFromMapListSection(const art::DexFile &dex_file,
                                              uint32_t start_offset,
                                              uint32_t count);

        StringId *GetStringId(uint32_t index) {
            CHECK_LT(index, StringIdsSize());
            return StringIds()[index].get();
        }

        TypeId *GetTypeId(uint32_t index) {
            CHECK_LT(index, TypeIdsSize());
            return TypeIds()[index].get();
        }

        ProtoId *GetProtoId(uint32_t index) {
            CHECK_LT(index, ProtoIdsSize());
            return ProtoIds()[index].get();
        }

        FieldId *GetFieldId(uint32_t index) {
            CHECK_LT(index, FieldIdsSize());
            return FieldIds()[index].get();
        }

        MethodId *GetMethodId(uint32_t index) {
            CHECK_LT(index, MethodIdsSize());
            return MethodIds()[index].get();
        }

        ClassDef *GetClassDef(uint32_t index) {
            CHECK_LT(index, ClassDefsSize());
            return ClassDefs()[index].get();
        }

        CallSiteId *GetCallSiteId(uint32_t index) {
            CHECK_LT(index, CallSiteIdsSize());
            return CallSiteIds()[index].get();
        }

        MethodHandleItem *GetMethodHandle(uint32_t index) {
            CHECK_LT(index, MethodHandleItemsSize());
            return MethodHandleItems()[index].get();
        }


        StringId *GetStringIdOrNullPtr(uint32_t index) {
            return index == art::kDexNoIndex ? nullptr : GetStringId(index);
        }

        TypeId *GetTypeIdOrNullPtr(uint16_t index) {
            return index == art::DexFile::kDexNoIndex16 ? nullptr : GetTypeId(index);
        }

        uint32_t StringIdsOffset() const { return string_ids_.GetOffset(); }

        uint32_t TypeIdsOffset() const { return type_ids_.GetOffset(); }

        uint32_t ProtoIdsOffset() const { return proto_ids_.GetOffset(); }

        uint32_t FieldIdsOffset() const { return field_ids_.GetOffset(); }

        uint32_t MethodIdsOffset() const { return method_ids_.GetOffset(); }

        uint32_t ClassDefsOffset() const { return class_defs_.GetOffset(); }

        uint32_t CallSiteIdsOffset() const { return call_site_ids_.GetOffset(); }

        uint32_t MethodHandleItemsOffset() const { return method_handle_items_.GetOffset(); }

        uint32_t StringDatasOffset() const { return string_datas_.GetOffset(); }

        uint32_t TypeListsOffset() const { return type_lists_.GetOffset(); }

        uint32_t EncodedArrayItemsOffset() const { return encoded_array_items_.GetOffset(); }

        uint32_t AnnotationItemsOffset() const { return annotation_items_.GetOffset(); }

        uint32_t AnnotationSetItemsOffset() const { return annotation_set_items_.GetOffset(); }

        uint32_t
        AnnotationSetRefListsOffset() const { return annotation_set_ref_lists_.GetOffset(); }

        uint32_t
        AnnotationsDirectoryItemsOffset() const { return annotations_directory_items_.GetOffset(); }

        uint32_t DebugInfoItemsOffset() const { return debug_info_items_.GetOffset(); }

        uint32_t CodeItemsOffset() const { return code_items_.GetOffset(); }

        uint32_t ClassDatasOffset() const { return class_datas_.GetOffset(); }

        uint32_t MapListOffset() const { return map_list_offset_; }

        void SetStringIdsOffset(uint32_t new_offset) { string_ids_.SetOffset(new_offset); }

        void SetTypeIdsOffset(uint32_t new_offset) { type_ids_.SetOffset(new_offset); }

        void SetProtoIdsOffset(uint32_t new_offset) { proto_ids_.SetOffset(new_offset); }

        void SetFieldIdsOffset(uint32_t new_offset) { field_ids_.SetOffset(new_offset); }

        void SetMethodIdsOffset(uint32_t new_offset) { method_ids_.SetOffset(new_offset); }

        void SetClassDefsOffset(uint32_t new_offset) { class_defs_.SetOffset(new_offset); }

        void SetCallSiteIdsOffset(uint32_t new_offset) { call_site_ids_.SetOffset(new_offset); }

        void SetMethodHandleItemsOffset(uint32_t new_offset) {
            method_handle_items_.SetOffset(new_offset);
        }

        void SetStringDatasOffset(uint32_t new_offset) { string_datas_.SetOffset(new_offset); }

        void SetTypeListsOffset(uint32_t new_offset) { type_lists_.SetOffset(new_offset); }

        void SetEncodedArrayItemsOffset(uint32_t new_offset) {
            encoded_array_items_.SetOffset(new_offset);
        }

        void SetAnnotationItemsOffset(uint32_t new_offset) {
            annotation_items_.SetOffset(new_offset);
        }

        void SetAnnotationSetItemsOffset(uint32_t new_offset) {
            annotation_set_items_.SetOffset(new_offset);
        }

        void SetAnnotationSetRefListsOffset(
                uint32_t new_offset) { annotation_set_ref_lists_.SetOffset(new_offset); }

        void SetAnnotationsDirectoryItemsOffset(
                uint32_t new_offset) { annotations_directory_items_.SetOffset(new_offset); }

        void SetDebugInfoItemsOffset(uint32_t new_offset) {
            debug_info_items_.SetOffset(new_offset);
        }

        void SetCodeItemsOffset(uint32_t new_offset) { code_items_.SetOffset(new_offset); }

        void SetClassDatasOffset(uint32_t new_offset) { class_datas_.SetOffset(new_offset); }

        void SetMapListOffset(uint32_t new_offset) { map_list_offset_ = new_offset; }

        uint32_t StringIdsSize() const { return string_ids_.Size(); }

        uint32_t TypeIdsSize() const { return type_ids_.Size(); }

        uint32_t ProtoIdsSize() const { return proto_ids_.Size(); }

        uint32_t FieldIdsSize() const { return field_ids_.Size(); }

        uint32_t MethodIdsSize() const { return method_ids_.Size(); }

        uint32_t ClassDefsSize() const { return class_defs_.Size(); }

        uint32_t CallSiteIdsSize() const { return call_site_ids_.Size(); }

        uint32_t MethodHandleItemsSize() const { return method_handle_items_.Size(); }

        uint32_t StringDatasSize() const { return string_datas_.Size(); }

        uint32_t TypeListsSize() const { return type_lists_.Size(); }

        uint32_t EncodedArrayItemsSize() const { return encoded_array_items_.Size(); }

        uint32_t AnnotationItemsSize() const { return annotation_items_.Size(); }

        uint32_t AnnotationSetItemsSize() const { return annotation_set_items_.Size(); }

        uint32_t AnnotationSetRefListsSize() const { return annotation_set_ref_lists_.Size(); }

        uint32_t
        AnnotationsDirectoryItemsSize() const { return annotations_directory_items_.Size(); }

        uint32_t DebugInfoItemsSize() const { return debug_info_items_.Size(); }

        uint32_t CodeItemsSize() const { return code_items_.Size(); }

        uint32_t MethodItemsSize() const {
            return method_items_.size();
        }

        uint32_t ClassDatasSize() const { return class_datas_.Size(); }

        // Sort the vectors buy map order (same order that was used in the input file).
        void SortVectorsByMapOrder();

        template<typename Type>
        void AddItem(CollectionMap<Type> &map,
                     CollectionVector<Type> &vector,
                     Type *item,
                     uint32_t offset);

        template<typename Type>
        void AddIndexedItem(IndexedCollectionVector<Type> &vector,
                            Type *item,
                            uint32_t offset,
                            uint32_t index);

        void SetEagerlyAssignOffsets(bool eagerly_assign_offsets) {
            eagerly_assign_offsets_ = eagerly_assign_offsets;
        }

        void SetLinkData(std::vector<uint8_t> &&link_data) {
            link_data_ = std::move(link_data);
        }

        const std::vector<uint8_t> &LinkData() const {
            return link_data_;
        }

        /* ~Collections(){
             method_items_.collection_.clear();
             __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, "method_items_ clear over");
         }*/

    private:
        EncodedValue *ReadEncodedValue(const art::DexFile &dex_file, const uint8_t **data);

        EncodedValue *ReadEncodedValue(const art::DexFile &dex_file,
                                       const uint8_t **data,
                                       uint8_t type,
                                       uint8_t length);

        void ReadEncodedValue(const art::DexFile &dex_file,
                              const uint8_t **data,
                              uint8_t type,
                              uint8_t length,
                              EncodedValue *item);

        ParameterAnnotation *
        GenerateParameterAnnotation(const art::DexFile &dex_file, MethodId *method_id,
                                    const art::DexFile::AnnotationSetRefList *annotation_set_ref_list,
                                    uint32_t offset);

        MethodItem *GenerateMethodItem(const art::DexFile &dex_file, art::ClassDataItemIterator &cdii);

        // Collection vectors own the IR data.
        IndexedCollectionVector<StringId> string_ids_;
        IndexedCollectionVector<TypeId> type_ids_;
        IndexedCollectionVector<ProtoId> proto_ids_;
        IndexedCollectionVector<FieldId> field_ids_;
        IndexedCollectionVector<MethodId> method_ids_;
        IndexedCollectionVector<CallSiteId> call_site_ids_;
        IndexedCollectionVector<MethodHandleItem> method_handle_items_;
        IndexedCollectionVector<StringData> string_datas_;
        IndexedCollectionVector<TypeList> type_lists_;
        IndexedCollectionVector<EncodedArrayItem> encoded_array_items_;
        IndexedCollectionVector<AnnotationItem> annotation_items_;
        IndexedCollectionVector<AnnotationSetItem> annotation_set_items_;
        IndexedCollectionVector<AnnotationSetRefList> annotation_set_ref_lists_;
        IndexedCollectionVector<AnnotationsDirectoryItem> annotations_directory_items_;
        IndexedCollectionVector<ClassDef> class_defs_;
        // The order of the vectors controls the layout of the output file by index order, to change the
        // layout just sort the vector. Note that you may only change the order of the non indexed vectors
        // below. Indexed vectors are accessed by indices in other places, changing the sorting order will
        // invalidate the existing indices and is not currently supported.
        CollectionVector<DebugInfoItem> debug_info_items_;
        std::map<uint32_t, MethodItem *> method_items_;
        CollectionVector<CodeItem> code_items_;
        CollectionVector<ClassData> class_datas_;

        // Note that the maps do not have ownership, the vectors do.
        // TODO: These maps should only be required for building the IR and should be put in a separate
        // IR builder class.
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

        uint32_t map_list_offset_ = 0;

        // Link data.
        std::vector<uint8_t> link_data_;

        // If we eagerly assign offsets during IR building or later after layout. Must be false if
        // changing the layout is enabled.
        bool eagerly_assign_offsets_;

        DISALLOW_COPY_AND_ASSIGN(Collections);
    };

}
#endif //UNPACKER_COLLECTIONS_H
