/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 1:26 AM
* @ class describe
*/

#include <inttypes.h>
#include <dex/dex_file.h>
#include <dex/dex_ir_header.h>
#include <dex/dex_ir_collections.h>
#include <inttypes.h>
namespace dex_ir {
    static void CheckAndSetRemainingOffsets(const art::DexFile &dex_file,
                                            Collections *collections);

    Header *DexIrBuilder(const art::DexFile &dex_file, bool eagerly_assign_offsets) {
        const art::DexFile::Header &disk_header = dex_file.GetHeader();
        Header *header = new Header(disk_header.magic_,
                                    disk_header.checksum_,
                                    disk_header.signature_,
                                    disk_header.endian_tag_,
                                    disk_header.file_size_,
                                    disk_header.header_size_,
                                    disk_header.link_size_,
                                    disk_header.link_off_,
                                    disk_header.data_size_,
                                    disk_header.data_off_,
                                    dex_file.SupportsDefaultMethods());
        //获取集合
        Collections &collections = header->GetCollections();
        //设置是
        collections.SetEagerlyAssignOffsets(eagerly_assign_offsets);
        // Walk the rest of the header fields.
        // StringId table.
        LOGI("[+] Builder start:%s", (eagerly_assign_offsets ? "true" : "false"));
        collections.SetStringIdsOffset(disk_header.string_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumStringIds(); ++i) {
            collections.CreateStringId(dex_file, i);
        }
        LOGI("[+] StringId:0x%x", collections.StringIds().size());
        // TypeId table.
        collections.SetTypeIdsOffset(disk_header.type_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumTypeIds(); ++i) {
            collections.CreateTypeId(dex_file, i);
        }
        LOGI("[+] TypeId:0x%x", collections.TypeIds().size());
        // ProtoId table.
        collections.SetProtoIdsOffset(disk_header.proto_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumProtoIds(); ++i) {
            collections.CreateProtoId(dex_file, i);
        }
        LOGI("[+] ProtoId:0x%x", collections.ProtoIds().size());
        // FieldId table.
        collections.SetFieldIdsOffset(disk_header.field_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumFieldIds(); ++i) {
            collections.CreateFieldId(dex_file, i);
        }
        LOGI("[+] FieldId:0x%x", collections.FieldIds().size());
        // MethodId table.
        collections.SetMethodIdsOffset(disk_header.method_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumMethodIds(); ++i) {
            collections.CreateMethodId(dex_file, i);
        }
        LOGI("[+] MethodId:0x%x", collections.MethodIds().size());
        // ClassDef table.
        collections.SetClassDefsOffset(disk_header.class_defs_off_);
        for (uint32_t i = 0; i < dex_file.NumClassDefs(); ++i) {
            collections.CreateClassDef(dex_file, i);
        }
        LOGI("[+] ClassDef:0x%x", collections.ClassDefs().size());
        // MapItem.
        collections.SetMapListOffset(disk_header.map_off_);
        // CallSiteIds and MethodHandleItems.
        collections.CreateCallSitesAndMethodHandles(dex_file);
        CheckAndSetRemainingOffsets(dex_file, &collections);

        // Sort the vectors by the map order (same order as the file).
        collections.SortVectorsByMapOrder();

        // Load the link data if it exists.
        collections.SetLinkData(std::vector<uint8_t>(
                dex_file.DataBegin() + dex_file.GetHeader().link_off_,
                dex_file.DataBegin() + dex_file.GetHeader().link_off_ +
                dex_file.GetHeader().link_size_));


        LOGI("[+] EncodedArrayItems:0x%x", collections.EncodedArrayItems().size());
        LOGI("[+] AnnotationItems:0x%x", collections.AnnotationItems().size());
        LOGI("[+] AnnotationSetItems:0x%x", collections.AnnotationSetItems().size());
        LOGI("[+] AnnotationSetRefLists:0x%x", collections.AnnotationSetRefLists().size());
        LOGI("[+] AnnotationsDirectoryItems:0x%x", collections.AnnotationsDirectoryItems().size());
        LOGI("[+] DebugInfoItems:0x%x", collections.DebugInfoItems().size());
        LOGI("[+] CodeItems:0x%x", collections.CodeItems().size());
        LOGI("[+] ClassDatas:0x%x", collections.ClassDatas().size());
        LOGI("[+] Split dex over");
        return header;
    }


    static void CheckAndSetRemainingOffsets(const art::DexFile &dex_file,
                                            Collections *collections) {
        const art::DexFile::Header &disk_header = dex_file.GetHeader();
        // Read MapItems and validate/set remaining offsets.
        const art::DexFile::MapList *map = dex_file.GetMapList();
        const uint32_t count = map->size_;
        for (uint32_t i = 0; i < count; ++i) {
            const art::DexFile::MapItem *item = map->list_ + i;
            switch (item->type_) {
                case art::DexFile::kDexTypeHeaderItem:
                    CHECK_EQ(item->size_, 1u);
                    CHECK_EQ(item->offset_, 0u);
                    break;
                case art::DexFile::kDexTypeStringIdItem:
                    CHECK_EQ(item->size_, collections->StringIdsSize());
                    CHECK_EQ(item->offset_, collections->StringIdsOffset());
                    break;
                case art::DexFile::kDexTypeTypeIdItem:
                    CHECK_EQ(item->size_, collections->TypeIdsSize());
                    CHECK_EQ(item->offset_, collections->TypeIdsOffset());
                    break;
                case art::DexFile::kDexTypeProtoIdItem:
                    CHECK_EQ(item->size_, collections->ProtoIdsSize());
                    CHECK_EQ(item->offset_, collections->ProtoIdsOffset());
                    break;
                case art::DexFile::kDexTypeFieldIdItem:
                    CHECK_EQ(item->size_, collections->FieldIdsSize());
                    CHECK_EQ(item->offset_, collections->FieldIdsOffset());
                    break;
                case art::DexFile::kDexTypeMethodIdItem:
                    CHECK_EQ(item->size_, collections->MethodIdsSize());
                    CHECK_EQ(item->offset_, collections->MethodIdsOffset());
                    break;
                case art::DexFile::kDexTypeClassDefItem:

                    CHECK_EQ(item->offset_, collections->ClassDefsOffset());
                    break;
                case art::DexFile::kDexTypeCallSiteIdItem:
                    CHECK_EQ(item->size_, collections->CallSiteIdsSize());
                    CHECK_EQ(item->offset_, collections->CallSiteIdsOffset());
                    break;
                case art::DexFile::kDexTypeMethodHandleItem:
                    CHECK_EQ(item->size_, collections->MethodHandleItemsSize());
                    CHECK_EQ(item->offset_, collections->MethodHandleItemsOffset());
                    break;
                case art::DexFile::kDexTypeMapList:
                    CHECK_EQ(item->size_, 1u);
                    CHECK_EQ(item->offset_, disk_header.map_off_);
                    break;
                case art::DexFile::kDexTypeTypeList:
                    collections->SetTypeListsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeAnnotationSetRefList:
                    collections->SetAnnotationSetRefListsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeAnnotationSetItem:
                    collections->SetAnnotationSetItemsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeClassDataItem:
                    collections->SetClassDatasOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeCodeItem:
                    collections->SetCodeItemsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeStringDataItem:
                    collections->SetStringDatasOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeDebugInfoItem:
                    collections->SetDebugInfoItemsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeAnnotationItem:
                    collections->SetAnnotationItemsOffset(item->offset_);
                    collections->AddAnnotationsFromMapListSection(dex_file, item->offset_,
                                                                  item->size_);
                    break;
                case art::DexFile::kDexTypeEncodedArrayItem:
                    collections->SetEncodedArrayItemsOffset(item->offset_);
                    break;
                case art::DexFile::kDexTypeAnnotationsDirectoryItem:
                    collections->SetAnnotationsDirectoryItemsOffset(item->offset_);
                    break;
                default:
                    __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__,
                                        "Unknown map list item type.");
            }
        }
    }

}
