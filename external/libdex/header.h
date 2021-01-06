//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_HEADER_H
#define BASE_HEADER_H

#include <string.h>
#include "dex/dex_file.h"
#include "item.h"
#include "dexir_util.h"
#include "indexed_collection_vector.h"
#include "string_data.h"
#include "string_id.h"
#include "type_id.h"
#include "type_list.h"
#include "proto_id.h"
#include "method_id.h"
#include "field_id.h"
#include "field_item.h"
#include "method_item.h"
#include "class_def.h"
#include "class_data.h"
#include "call_site_id.h"
#include "method_handle_item.h"
#include "annotation.h"
#include "builder_maps.h"
#include "debug_info_item.h"
#include "code_item.h"
#include "hiddenapi_class_data.h"


namespace dex_ir {
    class Decompilation;


    class Header : public Item {
    public:
        Header(const uint8_t *magic,
               uint32_t checksum,
               const uint8_t *signature,
               uint32_t endian_tag,
               uint32_t file_size,
               uint32_t header_size,
               uint32_t link_size,
               uint32_t link_offset,
               uint32_t data_size,
               uint32_t data_offset,
               bool support_default_methods)
                : Item(0, kHeaderItemSize), support_default_methods_(support_default_methods) {
            ConstructorHelper(magic,
                              checksum,
                              signature,
                              endian_tag,
                              file_size,
                              header_size,
                              link_size,
                              link_offset,
                              data_size,
                              data_offset);
        }

        Header(const uint8_t *magic,
               uint32_t checksum,
               const uint8_t *signature,
               uint32_t endian_tag,
               uint32_t file_size,
               uint32_t header_size,
               uint32_t link_size,
               uint32_t link_offset,
               uint32_t data_size,
               uint32_t data_offset,
               bool support_default_methods,
               uint32_t num_string_ids,
               uint32_t num_type_ids,
               uint32_t num_proto_ids,
               uint32_t num_field_ids,
               uint32_t num_method_ids,
               uint32_t num_class_defs)
                : Item(0, kHeaderItemSize),
                  support_default_methods_(support_default_methods),
                  string_ids_(num_string_ids),
                  type_ids_(num_type_ids),
                  proto_ids_(num_proto_ids),
                  field_ids_(num_field_ids),
                  method_ids_(num_method_ids),
                  class_defs_(num_class_defs) {
            ConstructorHelper(magic,
                              checksum,
                              signature,
                              endian_tag,
                              file_size,
                              header_size,
                              link_size,
                              link_offset,
                              data_size,
                              data_offset);
        }

        ~Header() override;

        static size_t ItemSize() { return kHeaderItemSize; }

        const uint8_t *Magic() const { return magic_; }

        uint32_t Checksum() const { return checksum_; }

        const uint8_t *Signature() const { return signature_; }

        uint32_t EndianTag() const { return endian_tag_; }

        uint32_t FileSize() const { return file_size_; }

        uint32_t HeaderSize() const { return header_size_; }

        uint32_t LinkSize() const { return link_size_; }

        uint32_t LinkOffset() const { return link_offset_; }

        uint32_t DataSize() const { return data_size_; }

        uint32_t DataOffset() const { return data_offset_; }

        void SetChecksum(uint32_t new_checksum) { checksum_ = new_checksum; }

        void SetSignature(const uint8_t *new_signature) {
            memcpy(signature_, new_signature, sizeof(signature_));
        }

        void SetFileSize(uint32_t new_file_size) { file_size_ = new_file_size; }

        void SetHeaderSize(uint32_t new_header_size) { header_size_ = new_header_size; }

        void SetLinkSize(uint32_t new_link_size) { link_size_ = new_link_size; }

        void SetLinkOffset(uint32_t new_link_offset) { link_offset_ = new_link_offset; }

        void SetDataSize(uint32_t new_data_size) { data_size_ = new_data_size; }

        void SetDataOffset(uint32_t new_data_offset) { data_offset_ = new_data_offset; }

        IndexedCollectionVector<StringId> &StringIds() { return string_ids_; }

        const IndexedCollectionVector<StringId> &StringIds() const { return string_ids_; }

        IndexedCollectionVector<TypeId> &TypeIds() { return type_ids_; }

        const IndexedCollectionVector<TypeId> &TypeIds() const { return type_ids_; }

        IndexedCollectionVector<ProtoId> &ProtoIds() { return proto_ids_; }

        const IndexedCollectionVector<ProtoId> &ProtoIds() const { return proto_ids_; }

        IndexedCollectionVector<FieldId> &FieldIds() { return field_ids_; }

        const IndexedCollectionVector<FieldId> &FieldIds() const { return field_ids_; }

        IndexedCollectionVector<MethodId> &MethodIds() { return method_ids_; }

        const IndexedCollectionVector<MethodId> &MethodIds() const { return method_ids_; }

        IndexedCollectionVector<ClassDef> &ClassDefs() { return class_defs_; }

        const IndexedCollectionVector<ClassDef> &ClassDefs() const { return class_defs_; }

        IndexedCollectionVector<CallSiteId> &CallSiteIds() { return call_site_ids_; }

        const IndexedCollectionVector<CallSiteId> &CallSiteIds() const { return call_site_ids_; }

        IndexedCollectionVector<MethodHandleItem> &MethodHandleItems() { return method_handle_items_; }

        const IndexedCollectionVector<MethodHandleItem> &MethodHandleItems() const {
            return method_handle_items_;
        }

        CollectionVector<StringData> &StringDatas() { return string_datas_; }

        const CollectionVector<StringData> &StringDatas() const { return string_datas_; }

        CollectionVector<TypeList> &TypeLists() { return type_lists_; }

        const CollectionVector<TypeList> &TypeLists() const { return type_lists_; }

        CollectionVector<EncodedArrayItem> &EncodedArrayItems() { return encoded_array_items_; }

        const CollectionVector<EncodedArrayItem> &EncodedArrayItems() const {
            return encoded_array_items_;
        }

        CollectionVector<AnnotationItem> &AnnotationItems() { return annotation_items_; }

        const CollectionVector<AnnotationItem> &AnnotationItems() const { return annotation_items_; }

        CollectionVector<AnnotationSetItem> &AnnotationSetItems() { return annotation_set_items_; }

        const CollectionVector<AnnotationSetItem> &AnnotationSetItems() const {
            return annotation_set_items_;
        }

        CollectionVector<AnnotationSetRefList> &AnnotationSetRefLists() {
            return annotation_set_ref_lists_;
        }

        const CollectionVector<AnnotationSetRefList> &AnnotationSetRefLists() const {
            return annotation_set_ref_lists_;
        }

        CollectionVector<AnnotationsDirectoryItem> &AnnotationsDirectoryItems() {
            return annotations_directory_items_;
        }

        const CollectionVector<AnnotationsDirectoryItem> &AnnotationsDirectoryItems() const {
            return annotations_directory_items_;
        }

        IndexedCollectionVector<HiddenapiClassData> &HiddenapiClassDatas() {
            return hiddenapi_class_datas_;
        }

        const IndexedCollectionVector<HiddenapiClassData> &HiddenapiClassDatas() const {
            return hiddenapi_class_datas_;
        }

        CollectionVector<DebugInfoItem> &DebugInfoItems() { return debug_info_items_; }

        const CollectionVector<DebugInfoItem> &DebugInfoItems() const { return debug_info_items_; }

        CollectionVector<CodeItem> &CodeItems() { return code_items_; }

        const CollectionVector<CodeItem> &CodeItems() const { return code_items_; }

        CollectionVector<ClassData> &ClassDatas() { return class_datas_; }

        const CollectionVector<ClassData> &ClassDatas() const { return class_datas_; }

        StringId *GetStringIdOrNullPtr(uint32_t index) {
            return index == libdex::dex::kDexNoIndex ? nullptr : StringIds()[index];
        }

        TypeId *GetTypeIdOrNullPtr(uint16_t index) {
            return index == libdex::DexFile::kDexNoIndex16 ? nullptr : TypeIds()[index];
        }

        uint32_t MapListOffset() const { return map_list_offset_; }

        void SetMapListOffset(uint32_t new_offset) { map_list_offset_ = new_offset; }

        const std::vector<uint8_t> &LinkData() const { return link_data_; }

        void SetLinkData(std::vector<uint8_t> &&link_data) { link_data_ = std::move(link_data); }

        //void Accept(AbstractDispatcher *dispatch) { dispatch->Dispatch(this); }

        bool SupportDefaultMethods() const {
            return support_default_methods_;
        }

        std::map<uint32_t, MethodItem *> &
        MethodItems() { return method_items_; }

        uint32_t MethodItemsSize() const {
            return method_items_.size();
        }

        void SetUpDecompilation();

        Decompilation *GetDecompilation();

        CodeItem *CreateCodeItem(const libdex::DexFile &dex_file, uint8_t *data, uint32_t off_in_dex,
                                 uint32_t dex_id_index);

    private:
        uint8_t magic_[8];
        uint32_t checksum_;
        uint8_t signature_[libdex::DexFile::kSha1DigestSize];
        uint32_t endian_tag_;
        uint32_t file_size_;
        uint32_t header_size_;
        uint32_t link_size_;
        uint32_t link_offset_;
        uint32_t data_size_;
        uint32_t data_offset_;
        const bool support_default_methods_;

        void ConstructorHelper(const uint8_t *magic,
                               uint32_t checksum,
                               const uint8_t *signature,
                               uint32_t endian_tag,
                               uint32_t file_size,
                               uint32_t header_size,
                               uint32_t link_size,
                               uint32_t link_offset,
                               uint32_t data_size,
                               uint32_t data_offset) {
            checksum_ = checksum;
            endian_tag_ = endian_tag;
            file_size_ = file_size;
            header_size_ = header_size;
            link_size_ = link_size;
            link_offset_ = link_offset;
            data_size_ = data_size;
            data_offset_ = data_offset;
            memcpy(magic_, magic, sizeof(magic_));
            memcpy(signature_, signature, sizeof(signature_));
        }

        // Collection vectors own the IR data.
        IndexedCollectionVector<StringId> string_ids_;
        IndexedCollectionVector<TypeId> type_ids_;
        IndexedCollectionVector<ProtoId> proto_ids_;
        IndexedCollectionVector<FieldId> field_ids_;
        IndexedCollectionVector<MethodId> method_ids_;
        IndexedCollectionVector<ClassDef> class_defs_;
        IndexedCollectionVector<CallSiteId> call_site_ids_;
        IndexedCollectionVector<MethodHandleItem> method_handle_items_;
        IndexedCollectionVector<StringData> string_datas_;
        IndexedCollectionVector<TypeList> type_lists_;
        IndexedCollectionVector<EncodedArrayItem> encoded_array_items_;
        IndexedCollectionVector<AnnotationItem> annotation_items_;
        IndexedCollectionVector<AnnotationSetItem> annotation_set_items_;
        IndexedCollectionVector<AnnotationSetRefList> annotation_set_ref_lists_;
        IndexedCollectionVector<AnnotationsDirectoryItem> annotations_directory_items_;
        IndexedCollectionVector<HiddenapiClassData> hiddenapi_class_datas_;
        // The order of the vectors controls the layout of the output file by index order, to change the
        // layout just sort the vector. Note that you may only change the order of the non indexed vectors
        // below. Indexed vectors are accessed by indices in other places, changing the sorting order will
        // invalidate the existing indices and is not currently supported.
        CollectionVector<DebugInfoItem> debug_info_items_;
        std::map<uint32_t, MethodItem *> method_items_;
        CollectionVector<CodeItem> code_items_;
        CollectionVector<ClassData> class_datas_;

        uint32_t map_list_offset_ = 0;

        // Link data.
        std::vector<uint8_t> link_data_;
        Decompilation *decompilation_;

    private:
        DISALLOW_COPY_AND_ASSIGN(Header);
    };

    dex_ir::Header *DexIrBuilder(const libdex::DexFile &dex_file,
                                 bool eagerly_assign_offsets);
}

#endif //BASE_HEADER_H
