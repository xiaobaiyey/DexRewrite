//
// Created by xiaob on 2020/1/1.
//

#include <libbase/leb128.h>
#include <libdex/dex/dex_file.h>
#include <libdex/dex/dex_file_exception_helpers.h>
#include "header.h"
#include "builder_maps.h"
#include "decompilation.h"
#include "dex/standard_dex_file.h"

namespace dex_ir {
    static uint32_t GetDebugInfoStreamSize_(const uint8_t *debug_info_stream) {
        const uint8_t *stream = debug_info_stream;
        base::DecodeUnsignedLeb128(&stream);  // line_start
        uint32_t parameters_size = base::DecodeUnsignedLeb128(&stream);
        for (uint32_t i = 0; i < parameters_size; ++i) {
            base::DecodeUnsignedLeb128P1(&stream);  // Parameter name.
        }

        for (;;) {
            uint8_t opcode = *stream++;
            switch (opcode) {
                case libdex::DexFile::DBG_END_SEQUENCE:
                    return stream - debug_info_stream;  // end of stream.
                case libdex::DexFile::DBG_ADVANCE_PC:
                    base::DecodeUnsignedLeb128(&stream);  // addr_diff
                    break;
                case libdex::DexFile::DBG_ADVANCE_LINE:
                    base::DecodeSignedLeb128(&stream);  // line_diff
                    break;
                case libdex::DexFile::DBG_START_LOCAL:
                    base::DecodeUnsignedLeb128(&stream);  // register_num
                    base::DecodeUnsignedLeb128P1(&stream);  // name_idx
                    base::DecodeUnsignedLeb128P1(&stream);  // type_idx
                    break;
                case libdex::DexFile::DBG_START_LOCAL_EXTENDED:
                    base::DecodeUnsignedLeb128(&stream);  // register_num
                    base::DecodeUnsignedLeb128P1(&stream);  // name_idx
                    base::DecodeUnsignedLeb128P1(&stream);  // type_idx
                    base::DecodeUnsignedLeb128P1(&stream);  // sig_idx
                    break;
                case libdex::DexFile::DBG_END_LOCAL:
                case libdex::DexFile::DBG_RESTART_LOCAL:
                    base::DecodeUnsignedLeb128(&stream);  // register_num
                    break;
                case libdex::DexFile::DBG_SET_PROLOGUE_END:
                case libdex::DexFile::DBG_SET_EPILOGUE_BEGIN:
                    break;
                case libdex::DexFile::DBG_SET_FILE: {
                    base::DecodeUnsignedLeb128P1(&stream);  // name_idx
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    dex_ir::Header *DexIrBuilder(const libdex::DexFile &dex_file,
                                 bool eagerly_assign_offsets) {

        const libdex::DexFile::Header &disk_header = dex_file.GetHeader();
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
                                    dex_file.SupportsDefaultMethods(),
                                    dex_file.NumStringIds(),
                                    dex_file.NumTypeIds(),
                                    dex_file.NumProtoIds(),
                                    dex_file.NumFieldIds(),
                                    dex_file.NumMethodIds(),
                                    dex_file.NumClassDefs());
        BuilderMaps builder_maps(header, eagerly_assign_offsets);
        // Walk the rest of the header fields.
        // StringId table.
        header->StringIds().SetOffset(disk_header.string_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumStringIds(); ++i) {
            builder_maps.CreateStringId(dex_file, i);
        }
        // TypeId table.
        header->TypeIds().SetOffset(disk_header.type_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumTypeIds(); ++i) {
            builder_maps.CreateTypeId(dex_file, i);
        }
        // ProtoId table.
        header->ProtoIds().SetOffset(disk_header.proto_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumProtoIds(); ++i) {
            builder_maps.CreateProtoId(dex_file, i);
        }
        // FieldId table.
        header->FieldIds().SetOffset(disk_header.field_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumFieldIds(); ++i) {
            builder_maps.CreateFieldId(dex_file, i);
        }
        // MethodId table.
        header->MethodIds().SetOffset(disk_header.method_ids_off_);
        for (uint32_t i = 0; i < dex_file.NumMethodIds(); ++i) {
            builder_maps.CreateMethodId(dex_file, i);
        }
        // ClassDef table.
        header->ClassDefs().SetOffset(disk_header.class_defs_off_);
        for (uint32_t i = 0; i < dex_file.NumClassDefs(); ++i) {
            builder_maps.CreateClassDef(dex_file, i);
        }
        // MapItem.
        header->SetMapListOffset(disk_header.map_off_);
        // CallSiteIds and MethodHandleItems.
        builder_maps.CreateCallSitesAndMethodHandles(dex_file);
        //builder_maps.CheckAndSetRemainingOffsets(dex_file, options);

        // Sort the vectors by the map order (same order as the file).
        builder_maps.SortVectorsByMapOrder();

        // Load the link data if it exists.
        header->SetLinkData(std::vector<uint8_t>(
                dex_file.DataBegin() + dex_file.GetHeader().link_off_,
                dex_file.DataBegin() + dex_file.GetHeader().link_off_ + dex_file.GetHeader().link_size_));
        header->SetUpDecompilation();
        return header;
    }

    void Header::SetUpDecompilation() {
        this->decompilation_ = new Decompilation(this);
    }

    Decompilation *Header::GetDecompilation() {
        return this->decompilation_;
    }

    Header::~Header() {
        delete (decompilation_);
    }

    CodeItem *
    Header::CreateCodeItem(const libdex::DexFile &dex_file, uint8_t *data, uint32_t off_in_dex, uint32_t dex_id_index) {
        if (data == nullptr) {
            return nullptr;
        }
        libdex::StandardDexFile::CodeItem *codeItem_dex = (libdex::StandardDexFile::CodeItem *) data;
        libdex::CodeItemDebugInfoAccessor accessor(dex_file, codeItem_dex, dex_id_index);
        const uint32_t debug_info_offset = accessor.DebugInfoOffset();
        const uint8_t *debug_info_stream = dex_file.GetDebugInfoStream(debug_info_offset);
        DebugInfoItem *debug_info = nullptr;
        if (debug_info_stream != nullptr) {
            uint32_t debug_info_size = GetDebugInfoStreamSize_(debug_info_stream);
            uint8_t *debug_info_buffer = new uint8_t[debug_info_size];
            memcpy(debug_info_buffer, debug_info_stream, debug_info_size);
            debug_info = this->DebugInfoItems().CreateAndAddItem(debug_info_size, debug_info_buffer);
        }
        uint32_t insns_size = accessor.InsnsSizeInCodeUnits();
        uint16_t *insns = new uint16_t[insns_size];
        memcpy(insns, accessor.Insns(), insns_size * sizeof(uint16_t));

        TryItemVector *tries = nullptr;
        CatchHandlerVector *handler_list = nullptr;
        if (accessor.TriesSize() > 0) {
            tries = new TryItemVector();
            handler_list = new CatchHandlerVector();
            for (const libdex::dex::TryItem &disk_try_item : accessor.TryItems()) {
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
                    for (libdex::CatchHandlerIterator it(accessor, disk_try_item); it.HasNext(); it.Next()) {
                        const libdex::dex::TypeIndex type_index = it.GetHandlerTypeIndex();
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
            uint32_t handlers_size = base::DecodeUnsignedLeb128(&handlers_data);
            while (handlers_size > handler_list->size()) {
                bool already_added = false;
                uint16_t handler_off = handlers_data - handlers_base;
                for (std::unique_ptr<const CatchHandler> &existing_handlers : *handler_list) {
                    if (handler_off == existing_handlers->GetListOffset()) {
                        already_added = true;
                        break;
                    }
                }
                int32_t size = base::DecodeSignedLeb128(&handlers_data);
                bool has_catch_all = size <= 0;
                if (has_catch_all) {
                    size = -size;
                }
                if (already_added) {
                    for (int32_t i = 0; i < size; i++) {
                        base::DecodeUnsignedLeb128(&handlers_data);
                        base::DecodeUnsignedLeb128(&handlers_data);
                    }
                    if (has_catch_all) {
                        base::DecodeUnsignedLeb128(&handlers_data);
                    }
                    continue;
                }
                TypeAddrPairVector *addr_pairs = new TypeAddrPairVector();
                for (int32_t i = 0; i < size; i++) {
                    const TypeId *type_id = GetTypeIdOrNullPtr(base::DecodeUnsignedLeb128(&handlers_data));
                    uint32_t addr = base::DecodeUnsignedLeb128(&handlers_data);
                    addr_pairs->push_back(
                            std::unique_ptr<const TypeAddrPair>(new TypeAddrPair(type_id, addr)));
                }
                if (has_catch_all) {
                    uint32_t addr = base::DecodeUnsignedLeb128(&handlers_data);
                    addr_pairs->push_back(
                            std::unique_ptr<const TypeAddrPair>(new TypeAddrPair(nullptr, addr)));
                }
                const CatchHandler *handler = new CatchHandler(has_catch_all, handler_off, addr_pairs);
                handler_list->push_back(std::unique_ptr<const CatchHandler>(handler));
            }
        }
        uint32_t size = dex_file.GetCodeItemSize(*codeItem_dex);
        CodeItem *code_item = CodeItems().CreateAndAddItem(accessor.RegistersSize(),
                                                           accessor.InsSize(),
                                                           accessor.OutsSize(),
                                                           debug_info,
                                                           insns_size,
                                                           insns,
                                                           tries,
                                                           handler_list);
        code_item->SetSize(size);
        return code_item;
    }


}