//
// Created by xiaobaiyey on 2019-07-14.
//

#include <zlib.h>
#include <fcntl.h>
#include <dex/dex_file.h>
#include <dex/standard_dex_file.h>
#include <dex/dex_instruction_iterator.h>
#include <base/bit_utils.h>
#include <util/fd_file.h>
#include "dex_utils.h"
#include "dex/leb128.h"
namespace art {
    static_assert(sizeof(StringIndex) == sizeof(uint32_t), "StringIndex size is wrong");
    static_assert(std::is_trivially_copyable<StringIndex>::value, "StringIndex not trivial");
    static_assert(sizeof(TypeIndex) == sizeof(uint16_t), "TypeIndex size is wrong");
    static_assert(std::is_trivially_copyable<TypeIndex>::value, "TypeIndex not trivial");


    uint32_t DexFile::CalculateChecksum() const {
        return CalculateChecksum(Begin(), Size());
    }

    uint32_t DexFile::CalculateChecksum(const uint8_t *begin, size_t size) {
        const uint32_t non_sum_bytes = OFFSETOF_MEMBER(DexFile::Header, signature_);
        return ChecksumMemoryRange(begin + non_sum_bytes, size - non_sum_bytes);
    }

    uint32_t DexFile::ChecksumMemoryRange(const uint8_t *begin, size_t size) {
        return adler32(adler32(0L, nullptr, 0), begin, size);
    }


    DexFile *DexFile::getDexFile(std::string path) {
        FdFile *fdFile = new FdFile(path, O_RDONLY, false);
        if (!fdFile->IsOpened())
            return nullptr;
        int64_t len = fdFile->GetLength();
        uint8_t *base = static_cast<uint8_t *>(malloc(len));
        bool read_result = fdFile->ReadFully(base, len);
        CHECK_EQ(read_result, true);
        int close_result = fdFile->Close();
        CHECK_EQ(close_result, 0);
        DexFile *dexFile = new StandardDexFile(base, len, path, 0, true);
        return dexFile;
    }

    DexFile *DexFile::getDexFile(const uint8_t *base,
                                 size_t size) {
        DexFile *dexFile = new StandardDexFile(base, size, std::string("null"), 0, false);
        return dexFile;
    }


    DexFile::DexFile(const uint8_t *base,
                     size_t size,
                     const uint8_t *data_begin,
                     size_t data_size,
                     const std::string &location,
                     uint32_t location_checksum,
                     bool is_compact_dex,
                     bool is_from_file_)
            : begin_(base),
              size_(size),
              data_begin_(data_begin),
              data_size_(data_size),
              location_(location),
              location_checksum_(location_checksum),
              header_(reinterpret_cast<const Header *>(base)),
              string_ids_(reinterpret_cast<const StringId *>(base + header_->string_ids_off_)),
              type_ids_(reinterpret_cast<const TypeId *>(base + header_->type_ids_off_)),
              field_ids_(reinterpret_cast<const FieldId *>(base + header_->field_ids_off_)),
              method_ids_(reinterpret_cast<const MethodId *>(base + header_->method_ids_off_)),
              proto_ids_(reinterpret_cast<const ProtoId *>(base + header_->proto_ids_off_)),
              class_defs_(reinterpret_cast<const ClassDef *>(base + header_->class_defs_off_)),
              method_handles_(nullptr),
              num_method_handles_(0),
              call_site_ids_(nullptr),
              num_call_site_ids_(0),
              is_compact_dex_(is_compact_dex),
              is_from_file(is_from_file_),
              is_platform_dex_(false) {
        CHECK(begin_ != nullptr) << GetLocation();
        CHECK_GT(size_, 0U) << GetLocation();
        // Check base (=header) alignment.
        // Must be 4-byte aligned to avoid undefined behavior when accessing
        // any of the sections via a pointer.
        CHECK_ALIGNED(begin_, alignof(Header));

        InitializeSectionsFromMapList();
    }

    DexFile::~DexFile() {
        // We don't call DeleteGlobalRef on dex_object_ because we're only called by DestroyJavaVM, and
        // that's only called after DetachCurrentThread, which means there's no JNIEnv. We could
        // re-attach, but cleaning up these global references is not obviously useful. It's not as if
        // the global reference table is otherwise empty!

        /**
         * no you should delete by yourself ,or will dlfree
         */
        if (this->is_from_file) {
            delete (begin_);
        }
    }

    bool DexFile::Init(std::string *error_msg) {
        if (!CheckMagicAndVersion(error_msg)) {
            return false;
        }
        return true;
    }

    bool DexFile::CheckMagicAndVersion(std::string *error_msg) const {
        if (!IsMagicValid()) {
            std::ostringstream oss;
            oss << "Unrecognized magic number in " << GetLocation() << ":"
                << " " << header_->magic_[0]
                << " " << header_->magic_[1]
                << " " << header_->magic_[2]
                << " " << header_->magic_[3];
            *error_msg = oss.str();
            return false;
        }
        if (!IsVersionValid()) {
            std::ostringstream oss;
            oss << "Unrecognized version number in " << GetLocation() << ":"
                << " " << header_->magic_[4]
                << " " << header_->magic_[5]
                << " " << header_->magic_[6]
                << " " << header_->magic_[7];
            *error_msg = oss.str();
            return false;
        }
        return true;
    }

    void DexFile::InitializeSectionsFromMapList() {
        const MapList *map_list = reinterpret_cast<const MapList *>(DataBegin() +
                                                                    header_->map_off_);
        if (header_->map_off_ == 0 || header_->map_off_ > DataSize()) {
            // Bad offset. The dex file verifier runs after this method and will reject the file.
            return;
        }
        const size_t count = map_list->size_;

        size_t map_limit = header_->map_off_ + count * sizeof(MapItem);
        if (header_->map_off_ >= map_limit || map_limit > DataSize()) {
            // Overflow or out out of bounds. The dex file verifier runs after
            // this method and will reject the file as it is malformed.
            return;
        }

        for (size_t i = 0; i < count; ++i) {
            const MapItem &map_item = map_list->list_[i];
            if (map_item.type_ == kDexTypeMethodHandleItem) {
                method_handles_ = reinterpret_cast<const MethodHandleItem *>(Begin() +
                                                                             map_item.offset_);
                num_method_handles_ = map_item.size_;
            } else if (map_item.type_ == kDexTypeCallSiteIdItem) {
                call_site_ids_ = reinterpret_cast<const CallSiteIdItem *>(Begin() +
                                                                          map_item.offset_);
                num_call_site_ids_ = map_item.size_;
            }
        }
    }

    uint32_t DexFile::Header::GetVersion() const {
        const char *version = reinterpret_cast<const char *>(&magic_[kDexMagicSize]);
        return atoi(version);
    }

    const DexFile::ClassDef *DexFile::FindClassDef(TypeIndex type_idx) const {
        size_t num_class_defs = NumClassDefs();
        // Fast path for rare no class defs case.
        if (num_class_defs == 0) {
            return nullptr;
        }
        for (size_t i = 0; i < num_class_defs; ++i) {
            const ClassDef &class_def = GetClassDef(i);
            if (class_def.class_idx_ == type_idx) {
                return &class_def;
            }
        }
        return nullptr;
    }

    uint32_t DexFile::FindCodeItemOffset(const DexFile::ClassDef &class_def,
                                         uint32_t method_idx) const {
        const uint8_t *class_data = GetClassData(class_def);
        CHECK(class_data != nullptr);
        ClassDataItemIterator it(*this, class_data);
        it.SkipAllFields();
        while (it.HasNextDirectMethod()) {
            if (it.GetMemberIndex() == method_idx) {
                return it.GetMethodCodeItemOffset();
            }
            it.Next();
        }
        while (it.HasNextVirtualMethod()) {
            if (it.GetMemberIndex() == method_idx) {
                return it.GetMethodCodeItemOffset();
            }
            it.Next();
        }
        __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, "Unable to find method %d", method_idx);
        UNREACHABLE();
    }

    const DexFile::FieldId *DexFile::FindFieldId(const DexFile::TypeId &declaring_klass,
                                                 const DexFile::StringId &name,
                                                 const DexFile::TypeId &type) const {
        // Binary search MethodIds knowing that they are sorted by class_idx, name_idx then proto_idx
        const TypeIndex class_idx = GetIndexForTypeId(declaring_klass);
        const StringIndex name_idx = GetIndexForStringId(name);
        const TypeIndex type_idx = GetIndexForTypeId(type);
        int32_t lo = 0;
        int32_t hi = NumFieldIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const DexFile::FieldId &field = GetFieldId(mid);
            if (class_idx > field.class_idx_) {
                lo = mid + 1;
            } else if (class_idx < field.class_idx_) {
                hi = mid - 1;
            } else {
                if (name_idx > field.name_idx_) {
                    lo = mid + 1;
                } else if (name_idx < field.name_idx_) {
                    hi = mid - 1;
                } else {
                    if (type_idx > field.type_idx_) {
                        lo = mid + 1;
                    } else if (type_idx < field.type_idx_) {
                        hi = mid - 1;
                    } else {
                        return &field;
                    }
                }
            }
        }
        return nullptr;
    }

    const DexFile::MethodId *DexFile::FindMethodId(const DexFile::TypeId &declaring_klass,
                                                   const DexFile::StringId &name,
                                                   const DexFile::ProtoId &signature) const {
        // Binary search MethodIds knowing that they are sorted by class_idx, name_idx then proto_idx
        const TypeIndex class_idx = GetIndexForTypeId(declaring_klass);
        const StringIndex name_idx = GetIndexForStringId(name);
        const uint16_t proto_idx = GetIndexForProtoId(signature);
        int32_t lo = 0;
        int32_t hi = NumMethodIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const DexFile::MethodId &method = GetMethodId(mid);
            if (class_idx > method.class_idx_) {
                lo = mid + 1;
            } else if (class_idx < method.class_idx_) {
                hi = mid - 1;
            } else {
                if (name_idx > method.name_idx_) {
                    lo = mid + 1;
                } else if (name_idx < method.name_idx_) {
                    hi = mid - 1;
                } else {
                    if (proto_idx > method.proto_idx_) {
                        lo = mid + 1;
                    } else if (proto_idx < method.proto_idx_) {
                        hi = mid - 1;
                    } else {
                        return &method;
                    }
                }
            }
        }
        return nullptr;
    }

    const DexFile::StringId *DexFile::FindStringId(const char *string) const {
        int32_t lo = 0;
        int32_t hi = NumStringIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const DexFile::StringId &str_id = GetStringId(StringIndex(mid));
            const char *str = GetStringData(str_id);
            int compare = CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(string, str);
            if (compare > 0) {
                lo = mid + 1;
            } else if (compare < 0) {
                hi = mid - 1;
            } else {
                return &str_id;
            }
        }
        return nullptr;
    }

    const DexFile::TypeId *DexFile::FindTypeId(const char *string) const {
        int32_t lo = 0;
        int32_t hi = NumTypeIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const TypeId &type_id = GetTypeId(TypeIndex(mid));
            const DexFile::StringId &str_id = GetStringId(type_id.descriptor_idx_);
            const char *str = GetStringData(str_id);
            int compare = CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(string, str);
            if (compare > 0) {
                lo = mid + 1;
            } else if (compare < 0) {
                hi = mid - 1;
            } else {
                return &type_id;
            }
        }
        return nullptr;
    }

    const DexFile::StringId *DexFile::FindStringId(const uint16_t *string, size_t length) const {
        int32_t lo = 0;
        int32_t hi = NumStringIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const DexFile::StringId &str_id = GetStringId(StringIndex(mid));
            const char *str = GetStringData(str_id);
            int compare = CompareModifiedUtf8ToUtf16AsCodePointValues(str, string, length);
            if (compare > 0) {
                lo = mid + 1;
            } else if (compare < 0) {
                hi = mid - 1;
            } else {
                return &str_id;
            }
        }
        return nullptr;
    }

    const DexFile::TypeId *DexFile::FindTypeId(StringIndex string_idx) const {
        int32_t lo = 0;
        int32_t hi = NumTypeIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const TypeId &type_id = GetTypeId(TypeIndex(mid));
            if (string_idx > type_id.descriptor_idx_) {
                lo = mid + 1;
            } else if (string_idx < type_id.descriptor_idx_) {
                hi = mid - 1;
            } else {
                return &type_id;
            }
        }
        return nullptr;
    }

    const DexFile::ProtoId *DexFile::FindProtoId(TypeIndex return_type_idx,
                                                 const TypeIndex *signature_type_idxs,
                                                 uint32_t signature_length) const {
        int32_t lo = 0;
        int32_t hi = NumProtoIds() - 1;
        while (hi >= lo) {
            int32_t mid = (hi + lo) / 2;
            const DexFile::ProtoId &proto = GetProtoId(mid);
            int compare = return_type_idx.index_ - proto.return_type_idx_.index_;
            if (compare == 0) {
                DexFileParameterIterator it(*this, proto);
                size_t i = 0;
                while (it.HasNext() && i < signature_length && compare == 0) {
                    compare = signature_type_idxs[i].index_ - it.GetTypeIdx().index_;
                    it.Next();
                    i++;
                }
                if (compare == 0) {
                    if (it.HasNext()) {
                        compare = -1;
                    } else if (i < signature_length) {
                        compare = 1;
                    }
                }
            }
            if (compare > 0) {
                lo = mid + 1;
            } else if (compare < 0) {
                hi = mid - 1;
            } else {
                return &proto;
            }
        }
        return nullptr;
    }

// Given a signature place the type ids into the given vector
    bool DexFile::CreateTypeList(const StringPiece &signature,
                                 TypeIndex *return_type_idx,
                                 std::vector<TypeIndex> *param_type_idxs) const {
        if (signature[0] != '(') {
            return false;
        }
        size_t offset = 1;
        size_t end = signature.size();
        bool process_return = false;
        while (offset < end) {
            size_t start_offset = offset;
            char c = signature[offset];
            offset++;
            if (c == ')') {
                process_return = true;
                continue;
            }
            while (c == '[') {  // process array prefix
                if (offset >= end) {  // expect some descriptor following [
                    return false;
                }
                c = signature[offset];
                offset++;
            }
            if (c == 'L') {  // process type descriptors
                do {
                    if (offset >= end) {  // unexpected early termination of descriptor
                        return false;
                    }
                    c = signature[offset];
                    offset++;
                } while (c != ';');
            }
            // TODO: avoid creating a std::string just to get a 0-terminated char array
            std::string descriptor(signature.data() + start_offset, offset - start_offset);
            const DexFile::TypeId *type_id = FindTypeId(descriptor.c_str());
            if (type_id == nullptr) {
                return false;
            }
            TypeIndex type_idx = GetIndexForTypeId(*type_id);
            if (!process_return) {
                param_type_idxs->push_back(type_idx);
            } else {
                *return_type_idx = type_idx;
                return offset == end;  // return true if the signature had reached a sensible end
            }
        }
        return false;  // failed to correctly parse return type
    }

    const Signature DexFile::CreateSignature(const StringPiece &signature) const {
        TypeIndex return_type_idx;
        std::vector<TypeIndex> param_type_indices;
        bool success = CreateTypeList(signature, &return_type_idx, &param_type_indices);
        if (!success) {
            return Signature::NoSignature();
        }
        const ProtoId *proto_id = FindProtoId(return_type_idx, param_type_indices);
        if (proto_id == nullptr) {
            return Signature::NoSignature();
        }
        return Signature(this, *proto_id);
    }

    int32_t DexFile::FindTryItem(const TryItem *try_items, uint32_t tries_size, uint32_t address) {
        uint32_t min = 0;
        uint32_t max = tries_size;
        while (min < max) {
            const uint32_t mid = (min + max) / 2;

            const DexFile::TryItem &ti = try_items[mid];
            const uint32_t start = ti.start_addr_;
            const uint32_t end = start + ti.insn_count_;

            if (address < start) {
                max = mid;
            } else if (address >= end) {
                min = mid + 1;
            } else {  // We have a winner!
                return mid;
            }
        }
        // No match.
        return -1;
    }

    bool DexFile::LineNumForPcCb(void *raw_context, const PositionInfo &entry) {
        LineNumFromPcContext *context = reinterpret_cast<LineNumFromPcContext *>(raw_context);

        // We know that this callback will be called in
        // ascending address order, so keep going until we find
        // a match or we've just gone past it.
        if (entry.address_ > context->address_) {
            // The line number from the previous positions callback
            // wil be the final result.
            return true;
        } else {
            context->line_num_ = entry.line_;
            return entry.address_ == context->address_;
        }
    }

// Read a signed integer.  "zwidth" is the zero-based byte count.
    int32_t DexFile::ReadSignedInt(const uint8_t *ptr, int zwidth) {
        int32_t val = 0;
        for (int i = zwidth; i >= 0; --i) {
            val = ((uint32_t) val >> 8) | (((int32_t) *ptr++) << 24);
        }
        val >>= (3 - zwidth) * 8;
        return val;
    }

// Read an unsigned integer.  "zwidth" is the zero-based byte count,
// "fill_on_right" indicates which side we want to zero-fill from.
    uint32_t DexFile::ReadUnsignedInt(const uint8_t *ptr, int zwidth, bool fill_on_right) {
        uint32_t val = 0;
        for (int i = zwidth; i >= 0; --i) {
            val = (val >> 8) | (((uint32_t) *ptr++) << 24);
        }
        if (!fill_on_right) {
            val >>= (3 - zwidth) * 8;
        }
        return val;
    }

// Read a signed long.  "zwidth" is the zero-based byte count.
    int64_t DexFile::ReadSignedLong(const uint8_t *ptr, int zwidth) {
        int64_t val = 0;
        for (int i = zwidth; i >= 0; --i) {
            val = ((uint64_t) val >> 8) | (((int64_t) *ptr++) << 56);
        }
        val >>= (7 - zwidth) * 8;
        return val;
    }

// Read an unsigned long.  "zwidth" is the zero-based byte count,
// "fill_on_right" indicates which side we want to zero-fill from.
    uint64_t DexFile::ReadUnsignedLong(const uint8_t *ptr, int zwidth, bool fill_on_right) {
        uint64_t val = 0;
        for (int i = zwidth; i >= 0; --i) {
            val = (val >> 8) | (((uint64_t) *ptr++) << 56);
        }
        if (!fill_on_right) {
            val >>= (7 - zwidth) * 8;
        }
        return val;
    }

    std::string DexFile::PrettyMethod(uint32_t method_idx, bool with_signature) const {
        if (method_idx >= NumMethodIds()) {
            return StringPrintf("<<invalid-method-idx-%d>>", method_idx);
        }
        const DexFile::MethodId &method_id = GetMethodId(method_idx);
        std::string result;
        const DexFile::ProtoId *proto_id = with_signature ? &GetProtoId(method_id.proto_idx_)
                                                          : nullptr;
        if (with_signature) {
            AppendPrettyDescriptor(StringByTypeIdx(proto_id->return_type_idx_), &result);
            result += ' ';
        }
        AppendPrettyDescriptor(GetMethodDeclaringClassDescriptor(method_id), &result);
        result += '.';
        result += GetMethodName(method_id);
        if (with_signature) {
            result += '(';
            const DexFile::TypeList *params = GetProtoParameters(*proto_id);
            if (params != nullptr) {
                const char *separator = "";
                for (uint32_t i = 0u, size = params->Size(); i != size; ++i) {
                    result += separator;
                    separator = ", ";
                    AppendPrettyDescriptor(StringByTypeIdx(params->GetTypeItem(i).type_idx_),
                                           &result);
                }
            }
            result += ')';
        }
        return result;
    }

    std::string DexFile::PrettyField(uint32_t field_idx, bool with_type) const {
        if (field_idx >= NumFieldIds()) {
            return StringPrintf("<<invalid-field-idx-%d>>", field_idx);
        }
        const DexFile::FieldId &field_id = GetFieldId(field_idx);
        std::string result;
        if (with_type) {
            result += GetFieldTypeDescriptor(field_id);
            result += ' ';
        }
        AppendPrettyDescriptor(GetFieldDeclaringClassDescriptor(field_id), &result);
        result += '.';
        result += GetFieldName(field_id);
        return result;
    }

    std::string DexFile::PrettyType(TypeIndex type_idx) const {
        if (type_idx.index_ >= NumTypeIds()) {
            return StringPrintf("<<invalid-type-idx-%d>>", type_idx.index_);
        }
        const DexFile::TypeId &type_id = GetTypeId(type_idx);
        return PrettyDescriptor(GetTypeDescriptor(type_id));
    }

// Checks that visibility is as expected. Includes special behavior for M and
// before to allow runtime and build visibility when expecting runtime.
    std::ostream &operator<<(std::ostream &os, const DexFile &dex_file) {
        os << StringPrintf("[DexFile: %s dex-checksum=%08x location-checksum=%08x %p-%p]",
                           dex_file.GetLocation().c_str(),
                           dex_file.GetHeader().checksum_, dex_file.GetLocationChecksum(),
                           dex_file.Begin(), dex_file.Begin() + dex_file.Size());
        return os;
    }

    std::string Signature::ToString() const {
        if (dex_file_ == nullptr) {
            CHECK(proto_id_ == nullptr);
            return "<no signature>";
        }
        const DexFile::TypeList *params = dex_file_->GetProtoParameters(*proto_id_);
        std::string result;
        if (params == nullptr) {
            result += "()";
        } else {
            result += "(";
            for (uint32_t i = 0; i < params->Size(); ++i) {
                result += dex_file_->StringByTypeIdx(params->GetTypeItem(i).type_idx_);
            }
            result += ")";
        }
        result += dex_file_->StringByTypeIdx(proto_id_->return_type_idx_);
        return result;
    }

    uint32_t Signature::GetNumberOfParameters() const {
        const DexFile::TypeList *params = dex_file_->GetProtoParameters(*proto_id_);
        return (params != nullptr) ? params->Size() : 0;
    }

    bool Signature::IsVoid() const {
        const char *return_type = dex_file_->GetReturnTypeDescriptor(*proto_id_);
        return strcmp(return_type, "V") == 0;
    }

    bool Signature::operator==(const StringPiece &rhs) const {
        if (dex_file_ == nullptr) {
            return false;
        }
        StringPiece tail(rhs);
        if (!tail.starts_with("(")) {
            return false;  // Invalid signature
        }
        tail.remove_prefix(1);  // "(";
        const DexFile::TypeList *params = dex_file_->GetProtoParameters(*proto_id_);
        if (params != nullptr) {
            for (uint32_t i = 0; i < params->Size(); ++i) {
                StringPiece param(dex_file_->StringByTypeIdx(params->GetTypeItem(i).type_idx_));
                if (!tail.starts_with(param)) {
                    return false;
                }
                tail.remove_prefix(param.length());
            }
        }
        if (!tail.starts_with(")")) {
            return false;
        }
        tail.remove_prefix(1);  // ")";
        return tail == dex_file_->StringByTypeIdx(proto_id_->return_type_idx_);
    }

    std::ostream &operator<<(std::ostream &os, const Signature &sig) {
        return os << sig.ToString();
    }

// Decodes the header section from the class data bytes.
    void ClassDataItemIterator::ReadClassDataHeader() {
        CHECK(ptr_pos_ != nullptr);
        header_.static_fields_size_ = DecodeUnsignedLeb128(&ptr_pos_);
        header_.instance_fields_size_ = DecodeUnsignedLeb128(&ptr_pos_);
        header_.direct_methods_size_ = DecodeUnsignedLeb128(&ptr_pos_);
        header_.virtual_methods_size_ = DecodeUnsignedLeb128(&ptr_pos_);
    }

    void ClassDataItemIterator::ReadClassDataField() {
        field_.field_idx_delta_ = DecodeUnsignedLeb128(&ptr_pos_);
        field_.access_flags_ = DecodeUnsignedLeb128(&ptr_pos_);
        // The user of the iterator is responsible for checking if there
        // are unordered or duplicate indexes.
    }

    void ClassDataItemIterator::ReadClassDataMethod() {
        method_.method_idx_delta_ = DecodeUnsignedLeb128(&ptr_pos_);
        method_.access_flags_ = DecodeUnsignedLeb128(&ptr_pos_);
        method_.code_off_ = DecodeUnsignedLeb128(&ptr_pos_);
        if (last_idx_ != 0 && method_.method_idx_delta_ == 0) {
            __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, "Duplicate method in %s",
                                dex_file_.GetLocation().c_str());
        }
    }

    EncodedArrayValueIterator::EncodedArrayValueIterator(const DexFile &dex_file,
                                                         const uint8_t *array_data)
            : dex_file_(dex_file),
              array_size_(),
              pos_(-1),
              ptr_(array_data),
              type_(kByte) {
        array_size_ = (ptr_ != nullptr) ? DecodeUnsignedLeb128(&ptr_) : 0;
        if (array_size_ > 0) {
            Next();
        }
    }

    void EncodedArrayValueIterator::Next() {
        pos_++;
        if (pos_ >= array_size_) {
            return;
        }
        uint8_t value_type = *ptr_++;
        uint8_t value_arg = value_type >> kEncodedValueArgShift;
        size_t width = value_arg + 1;  // assume and correct later
        type_ = static_cast<ValueType>(value_type & kEncodedValueTypeMask);
        switch (type_) {
            case kBoolean:
                jval_.i = (value_arg != 0) ? 1 : 0;
                width = 0;
                break;
            case kByte:
                jval_.i = DexFile::ReadSignedInt(ptr_, value_arg);
                CHECK(IsInt<8>(jval_.i));
                break;
            case kShort:
                jval_.i = DexFile::ReadSignedInt(ptr_, value_arg);
                CHECK(IsInt<16>(jval_.i));
                break;
            case kChar:
                jval_.i = DexFile::ReadUnsignedInt(ptr_, value_arg, false);
                CHECK(IsUint<16>(jval_.i));
                break;
            case kInt:
                jval_.i = DexFile::ReadSignedInt(ptr_, value_arg);
                break;
            case kLong:
                jval_.j = DexFile::ReadSignedLong(ptr_, value_arg);
                break;
            case kFloat:
                jval_.i = DexFile::ReadUnsignedInt(ptr_, value_arg, true);
                break;
            case kDouble:
                jval_.j = DexFile::ReadUnsignedLong(ptr_, value_arg, true);
                break;
            case kString:
            case kType:
            case kMethodType:
            case kMethodHandle:
                jval_.i = DexFile::ReadUnsignedInt(ptr_, value_arg, false);
                break;
            case kField:
            case kMethod:
            case kEnum:
            case kArray:
            case kAnnotation:
                //UNIMPLEMENTED(FATAL) << ": type " << type_;
                UNREACHABLE();
            case kNull:
                jval_.l = nullptr;
                width = 0;
                break;
            default:
                __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__, "Unreached");
                UNREACHABLE();
        }
        ptr_ += width;
    }


    std::ostream &operator<<(std::ostream &os, const StringIndex &index) {
        os << "StringIndex[" << index.index_ << "]";
        return os;
    }

    std::ostream &operator<<(std::ostream &os, const TypeIndex &index) {
        os << "TypeIndex[" << index.index_ << "]";
        return os;
    }


    int32_t DexFile::GetStringLength(const StringId &string_id) const {
        const uint8_t *ptr = DataBegin() + string_id.string_data_off_;
        return DecodeUnsignedLeb128(&ptr);
    }

    const char *DexFile::GetStringDataAndUtf16Length(const StringId &string_id,
                                                     uint32_t *utf16_length) const {
        DCHECK(utf16_length != nullptr) << GetLocation();
        const uint8_t *ptr = DataBegin() + string_id.string_data_off_;
        *utf16_length = DecodeUnsignedLeb128(&ptr);
        return reinterpret_cast<const char *>(ptr);
    }

    const char *DexFile::GetStringData(const StringId &string_id) const {
        uint32_t ignored;
        return GetStringDataAndUtf16Length(string_id, &ignored);
    }

    const char *DexFile::StringDataAndUtf16LengthByIdx(StringIndex idx,
                                                       uint32_t *utf16_length) const {
        if (!idx.IsValid()) {
            *utf16_length = 0;
            return nullptr;
        }
        const StringId &string_id = GetStringId(idx);
        return GetStringDataAndUtf16Length(string_id, utf16_length);
    }

    const char *DexFile::StringDataByIdx(StringIndex idx) const {
        uint32_t unicode_length;
        return StringDataAndUtf16LengthByIdx(idx, &unicode_length);
    }

    const char *DexFile::StringByTypeIdx(TypeIndex idx, uint32_t *unicode_length) const {
        if (!idx.IsValid()) {
            return nullptr;
        }
        const TypeId &type_id = GetTypeId(idx);
        return StringDataAndUtf16LengthByIdx(type_id.descriptor_idx_, unicode_length);
    }

    const char *DexFile::StringByTypeIdx(TypeIndex idx) const {
        if (!idx.IsValid()) {
            return nullptr;
        }
        const TypeId &type_id = GetTypeId(idx);
        return StringDataByIdx(type_id.descriptor_idx_);
    }

    const char *DexFile::GetTypeDescriptor(const TypeId &type_id) const {
        return StringDataByIdx(type_id.descriptor_idx_);
    }

    const char *DexFile::GetFieldTypeDescriptor(const FieldId &field_id) const {
        const DexFile::TypeId &type_id = GetTypeId(field_id.type_idx_);
        return GetTypeDescriptor(type_id);
    }

    const char *DexFile::GetFieldName(const FieldId &field_id) const {
        return StringDataByIdx(field_id.name_idx_);
    }

    const char *DexFile::GetMethodDeclaringClassDescriptor(const MethodId &method_id) const {
        const DexFile::TypeId &type_id = GetTypeId(method_id.class_idx_);
        return GetTypeDescriptor(type_id);
    }

    const Signature DexFile::GetMethodSignature(const MethodId &method_id) const {
        return Signature(this, GetProtoId(method_id.proto_idx_));
    }

    const Signature DexFile::GetProtoSignature(const ProtoId &proto_id) const {
        return Signature(this, proto_id);
    }

    const char *DexFile::GetMethodName(const MethodId &method_id) const {
        return StringDataByIdx(method_id.name_idx_);
    }

    const char *DexFile::GetMethodShorty(uint32_t idx) const {
        return StringDataByIdx(GetProtoId(GetMethodId(idx).proto_idx_).shorty_idx_);
    }

    const char *DexFile::GetMethodShorty(const MethodId &method_id) const {
        return StringDataByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_);
    }

    const char *DexFile::GetMethodShorty(const MethodId &method_id, uint32_t *length) const {
        // Using the UTF16 length is safe here as shorties are guaranteed to be ASCII characters.
        return StringDataAndUtf16LengthByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_, length);
    }

    const char *DexFile::GetClassDescriptor(const ClassDef &class_def) const {
        return StringByTypeIdx(class_def.class_idx_);
    }

    const char *DexFile::GetReturnTypeDescriptor(const ProtoId &proto_id) const {
        return StringByTypeIdx(proto_id.return_type_idx_);
    }

    const char *DexFile::GetShorty(uint32_t proto_idx) const {
        const ProtoId &proto_id = GetProtoId(proto_idx);
        return StringDataByIdx(proto_id.shorty_idx_);
    }

    const DexFile::TryItem *DexFile::GetTryItems(const DexInstructionIterator &code_item_end,
                                                 uint32_t offset) {
        return reinterpret_cast<const TryItem *>
               (RoundUp(reinterpret_cast<uintptr_t>(&code_item_end.Inst()), TryItem::kAlignment)) + offset;
    }

    static bool DexFileStringEquals(const DexFile *df1, StringIndex sidx1,
                                    const DexFile *df2, StringIndex sidx2) {
        uint32_t s1_len;  // Note: utf16 length != mutf8 length.
        const char *s1_data = df1->StringDataAndUtf16LengthByIdx(sidx1, &s1_len);
        uint32_t s2_len;
        const char *s2_data = df2->StringDataAndUtf16LengthByIdx(sidx2, &s2_len);
        return (s1_len == s2_len) && (strcmp(s1_data, s2_data) == 0);
    }

    bool Signature::operator==(const Signature &rhs) const {
        if (dex_file_ == nullptr) {
            return rhs.dex_file_ == nullptr;
        }
        if (rhs.dex_file_ == nullptr) {
            return false;
        }
        if (dex_file_ == rhs.dex_file_) {
            return proto_id_ == rhs.proto_id_;
        }
        uint32_t lhs_shorty_len;  // For a shorty utf16 length == mutf8 length.
        const char *lhs_shorty_data = dex_file_->StringDataAndUtf16LengthByIdx(proto_id_->shorty_idx_,
                                                                               &lhs_shorty_len);
        StringPiece lhs_shorty(lhs_shorty_data, lhs_shorty_len);
        {
            uint32_t rhs_shorty_len;
            const char *rhs_shorty_data =
                    rhs.dex_file_->StringDataAndUtf16LengthByIdx(rhs.proto_id_->shorty_idx_,
                                                                 &rhs_shorty_len);
            StringPiece rhs_shorty(rhs_shorty_data, rhs_shorty_len);
            if (lhs_shorty != rhs_shorty) {
                return false;  // Shorty mismatch.
            }
        }
        if (lhs_shorty[0] == 'L') {
            const DexFile::TypeId &return_type_id = dex_file_->GetTypeId(proto_id_->return_type_idx_);
            const DexFile::TypeId &rhs_return_type_id =
                    rhs.dex_file_->GetTypeId(rhs.proto_id_->return_type_idx_);
            if (!DexFileStringEquals(dex_file_, return_type_id.descriptor_idx_,
                                     rhs.dex_file_, rhs_return_type_id.descriptor_idx_)) {
                return false;  // Return type mismatch.
            }
        }
        if (lhs_shorty.find('L', 1) != StringPiece::npos) {
            const DexFile::TypeList *params = dex_file_->GetProtoParameters(*proto_id_);
            const DexFile::TypeList *rhs_params = rhs.dex_file_->GetProtoParameters(*rhs.proto_id_);
            // We found a reference parameter in the matching shorty, so both lists must be non-empty.
            DCHECK(params != nullptr);
            DCHECK(rhs_params != nullptr);
            uint32_t params_size = params->Size();
            DCHECK_EQ(params_size, rhs_params->Size());  // Parameter list size must match.
            for (uint32_t i = 0; i < params_size; ++i) {
                const DexFile::TypeId &param_id = dex_file_->GetTypeId(params->GetTypeItem(i).type_idx_);
                const DexFile::TypeId &rhs_param_id =
                        rhs.dex_file_->GetTypeId(rhs_params->GetTypeItem(i).type_idx_);
                if (!DexFileStringEquals(dex_file_, param_id.descriptor_idx_,
                                         rhs.dex_file_, rhs_param_id.descriptor_idx_)) {
                    return false;  // Parameter type mismatch.
                }
            }
        }
        return true;
    }


    InvokeType ClassDataItemIterator::GetMethodInvokeType(const DexFile::ClassDef &class_def) const {
        if (HasNextDirectMethod()) {
            if ((GetRawMemberAccessFlags() & kAccStatic) != 0) {
                return kStatic;
            } else {
                return kDirect;
            }
        } else {
            DCHECK_EQ(GetRawMemberAccessFlags() & kAccStatic, 0U);
            if ((class_def.access_flags_ & kAccInterface) != 0) {
                return kInterface;
            } else if ((GetRawMemberAccessFlags() & kAccConstructor) != 0) {
                return kSuper;
            } else {
                return kVirtual;
            }
        }
    }

    template<typename NewLocalCallback, typename IndexToStringData, typename TypeIndexToStringData>
    bool DexFile::DecodeDebugLocalInfo(const uint8_t *stream,
                                       const std::string &location,
                                       const char *declaring_class_descriptor,
                                       const std::vector<const char *> &arg_descriptors,
                                       const std::string &method_name,
                                       bool is_static,
                                       uint16_t registers_size,
                                       uint16_t ins_size,
                                       uint16_t insns_size_in_code_units,
                                       IndexToStringData index_to_string_data,
                                       TypeIndexToStringData type_index_to_string_data,
                                       NewLocalCallback new_local_callback,
                                       void *context) {
        if (stream == nullptr) {
            return false;
        }
        std::vector<LocalInfo> local_in_reg(registers_size);

        uint16_t arg_reg = registers_size - ins_size;
        if (!is_static) {
            const char *descriptor = declaring_class_descriptor;
            local_in_reg[arg_reg].name_ = "this";
            local_in_reg[arg_reg].descriptor_ = descriptor;
            local_in_reg[arg_reg].signature_ = nullptr;
            local_in_reg[arg_reg].start_address_ = 0;
            local_in_reg[arg_reg].reg_ = arg_reg;
            local_in_reg[arg_reg].is_live_ = true;
            arg_reg++;
        }

        DecodeUnsignedLeb128(&stream);  // Line.
        uint32_t parameters_size = DecodeUnsignedLeb128(&stream);
        uint32_t i;
        if (parameters_size != arg_descriptors.size()) {
            __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__,
                                "invalid stream - problem with parameter iterator in %s  for method %s",
                                location.c_str(), method_name.c_str());

            return false;
        }
        for (i = 0; i < parameters_size && i < arg_descriptors.size(); ++i) {
            if (arg_reg >= registers_size) {
                __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__,
                                    "invalid stream - arg reg >= reg size ( %d  >= %d ) in %s", arg_reg, registers_size,
                                    location.c_str());
                return false;
            }
            uint32_t name_idx = DecodeUnsignedLeb128P1(&stream);
            const char *descriptor = arg_descriptors[i];
            local_in_reg[arg_reg].name_ = index_to_string_data(name_idx);
            local_in_reg[arg_reg].descriptor_ = descriptor;
            local_in_reg[arg_reg].signature_ = nullptr;
            local_in_reg[arg_reg].start_address_ = 0;
            local_in_reg[arg_reg].reg_ = arg_reg;
            local_in_reg[arg_reg].is_live_ = true;
            switch (*descriptor) {
                case 'D':
                case 'J':
                    arg_reg += 2;
                    break;
                default:
                    arg_reg += 1;
                    break;
            }
        }

        uint32_t address = 0;
        for (;;) {
            uint8_t opcode = *stream++;
            switch (opcode) {
                case DBG_END_SEQUENCE:
                    // Emit all variables which are still alive at the end of the method.
                    for (uint16_t reg = 0; reg < registers_size; reg++) {
                        if (local_in_reg[reg].is_live_) {
                            local_in_reg[reg].end_address_ = insns_size_in_code_units;
                            new_local_callback(context, local_in_reg[reg]);
                        }
                    }
                    return true;
                case DBG_ADVANCE_PC:
                    address += DecodeUnsignedLeb128(&stream);
                    break;
                case DBG_ADVANCE_LINE:
                    DecodeSignedLeb128(&stream);  // Line.
                    break;
                case DBG_START_LOCAL:
                case DBG_START_LOCAL_EXTENDED: {
                    uint16_t reg = DecodeUnsignedLeb128(&stream);
                    if (reg >= registers_size) {
                        __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__,
                                            "invalid stream - arg reg >= reg size ( %d  >= %d ) in %s", arg_reg,
                                            registers_size, location.c_str());

                        return false;
                    }

                    uint32_t name_idx = DecodeUnsignedLeb128P1(&stream);
                    uint16_t descriptor_idx = DecodeUnsignedLeb128P1(&stream);
                    uint32_t signature_idx = kDexNoIndex;
                    if (opcode == DBG_START_LOCAL_EXTENDED) {
                        signature_idx = DecodeUnsignedLeb128P1(&stream);
                    }

                    // Emit what was previously there, if anything
                    if (local_in_reg[reg].is_live_) {
                        local_in_reg[reg].end_address_ = address;
                        new_local_callback(context, local_in_reg[reg]);
                    }

                    local_in_reg[reg].name_ = index_to_string_data(name_idx);
                    local_in_reg[reg].descriptor_ = type_index_to_string_data(descriptor_idx);;
                    local_in_reg[reg].signature_ = index_to_string_data(signature_idx);
                    local_in_reg[reg].start_address_ = address;
                    local_in_reg[reg].reg_ = reg;
                    local_in_reg[reg].is_live_ = true;
                    break;
                }
                case DBG_END_LOCAL: {
                    uint16_t reg = DecodeUnsignedLeb128(&stream);
                    if (reg >= registers_size) {
                        __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__,
                                            "invalid stream - arg reg >= reg size ( %d  >= %d ) in %s", arg_reg,
                                            registers_size, location.c_str());

                        return false;
                    }
                    // If the register is live, close it properly. Otherwise, closing an already
                    // closed register is sloppy, but harmless if no further action is taken.
                    if (local_in_reg[reg].is_live_) {
                        local_in_reg[reg].end_address_ = address;
                        new_local_callback(context, local_in_reg[reg]);
                        local_in_reg[reg].is_live_ = false;
                    }
                    break;
                }
                case DBG_RESTART_LOCAL: {
                    uint16_t reg = DecodeUnsignedLeb128(&stream);
                    if (reg >= registers_size) {
                        __android_log_print(ANDROID_LOG_ERROR, __FUNCTION__,
                                            "invalid stream - arg reg >= reg size ( %d  >= %d ) in %s", arg_reg,
                                            registers_size, location.c_str());

                        return false;
                    }
                    // If the register is live, the "restart" is superfluous,
                    // and we don't want to mess with the existing start address.
                    if (!local_in_reg[reg].is_live_) {
                        local_in_reg[reg].start_address_ = address;
                        local_in_reg[reg].is_live_ = true;
                    }
                    break;
                }
                case DBG_SET_PROLOGUE_END:
                case DBG_SET_EPILOGUE_BEGIN:
                    break;
                case DBG_SET_FILE:
                    DecodeUnsignedLeb128P1(&stream);  // name.
                    break;
                default:
                    address += (opcode - DBG_FIRST_SPECIAL) / DBG_LINE_RANGE;
                    break;
            }
        }
    }

    template<typename NewLocalCallback>
    bool DexFile::DecodeDebugLocalInfo(uint32_t registers_size,
                                       uint32_t ins_size,
                                       uint32_t insns_size_in_code_units,
                                       uint32_t debug_info_offset,
                                       bool is_static,
                                       uint32_t method_idx,
                                       NewLocalCallback new_local_callback,
                                       void *context) const {
        const uint8_t *const stream = GetDebugInfoStream(debug_info_offset);
        if (stream == nullptr) {
            return false;
        }
        std::vector<const char *> arg_descriptors;
        DexFileParameterIterator it(*this, GetMethodPrototype(GetMethodId(method_idx)));
        for (; it.HasNext(); it.Next()) {
            arg_descriptors.push_back(it.GetDescriptor());
        }
        return DecodeDebugLocalInfo(stream,
                                    GetLocation(),
                                    GetMethodDeclaringClassDescriptor(GetMethodId(method_idx)),
                                    arg_descriptors,
                                    this->PrettyMethod(method_idx),
                                    is_static,
                                    registers_size,
                                    ins_size,
                                    insns_size_in_code_units,
                                    [this](uint32_t idx) {
                                        return StringDataByIdx(StringIndex(idx));
                                    },
                                    [this](uint32_t idx) {
                                        return StringByTypeIdx(TypeIndex(
                                                dchecked_integral_cast<uint16_t>(idx)));
                                    },
                                    new_local_callback,
                                    context);
    }

    template<typename DexDebugNewPosition, typename IndexToStringData>
    bool DexFile::DecodeDebugPositionInfo(const uint8_t *stream,
                                          IndexToStringData index_to_string_data,
                                          DexDebugNewPosition position_functor,
                                          void *context) {
        if (stream == nullptr) {
            return false;
        }

        PositionInfo entry = PositionInfo();
        entry.line_ = DecodeUnsignedLeb128(&stream);
        uint32_t parameters_size = DecodeUnsignedLeb128(&stream);
        for (uint32_t i = 0; i < parameters_size; ++i) {
            DecodeUnsignedLeb128P1(&stream);  // Parameter name.
        }

        for (;;) {
            uint8_t opcode = *stream++;
            switch (opcode) {
                case DBG_END_SEQUENCE:
                    return true;  // end of stream.
                case DBG_ADVANCE_PC:
                    entry.address_ += DecodeUnsignedLeb128(&stream);
                    break;
                case DBG_ADVANCE_LINE:
                    entry.line_ += DecodeSignedLeb128(&stream);
                    break;
                case DBG_START_LOCAL:
                    DecodeUnsignedLeb128(&stream);  // reg.
                    DecodeUnsignedLeb128P1(&stream);  // name.
                    DecodeUnsignedLeb128P1(&stream);  // descriptor.
                    break;
                case DBG_START_LOCAL_EXTENDED:
                    DecodeUnsignedLeb128(&stream);  // reg.
                    DecodeUnsignedLeb128P1(&stream);  // name.
                    DecodeUnsignedLeb128P1(&stream);  // descriptor.
                    DecodeUnsignedLeb128P1(&stream);  // signature.
                    break;
                case DBG_END_LOCAL:
                case DBG_RESTART_LOCAL:
                    DecodeUnsignedLeb128(&stream);  // reg.
                    break;
                case DBG_SET_PROLOGUE_END:
                    entry.prologue_end_ = true;
                    break;
                case DBG_SET_EPILOGUE_BEGIN:
                    entry.epilogue_begin_ = true;
                    break;
                case DBG_SET_FILE: {
                    uint32_t name_idx = DecodeUnsignedLeb128P1(&stream);
                    entry.source_file_ = index_to_string_data(name_idx);
                    break;
                }
                default: {
                    int adjopcode = opcode - DBG_FIRST_SPECIAL;
                    entry.address_ += adjopcode / DBG_LINE_RANGE;
                    entry.line_ += DBG_LINE_BASE + (adjopcode % DBG_LINE_RANGE);
                    if (position_functor(context, entry)) {
                        return true;  // early exit.
                    }
                    entry.prologue_end_ = false;
                    entry.epilogue_begin_ = false;
                    break;
                }
            }
        }
    }

    template<typename DexDebugNewPosition>
    bool DexFile::DecodeDebugPositionInfo(uint32_t debug_info_offset,
                                          DexDebugNewPosition position_functor,
                                          void *context) const {
        return DecodeDebugPositionInfo(GetDebugInfoStream(debug_info_offset),
                                       [this](uint32_t idx) {
                                           return StringDataByIdx(StringIndex(idx));
                                       },
                                       position_functor,
                                       context);
    }


    const StandardDexFile *DexFile::AsStandardDexFile() const {
        DCHECK(IsStandardDexFile());
        return down_cast<const StandardDexFile *>(this);
    }

// Get the base of the encoded data for the given DexCode.
    const uint8_t *DexFile::GetCatchHandlerData(const DexInstructionIterator &code_item_end,
                                                uint32_t tries_size,
                                                uint32_t offset) {
        const uint8_t *handler_data =
                reinterpret_cast<const uint8_t *>(GetTryItems(code_item_end, tries_size));
        return handler_data + offset;
    }

    template<typename Visitor>
    void DexFile::ClassDef::VisitMethods(const DexFile *dex_file, const Visitor &visitor) const {
        const uint8_t *class_data = dex_file->GetClassData(*this);
        if (class_data != nullptr) {
            ClassDataItemIterator it(*dex_file, class_data);
            it.SkipAllFields();
            for (; it.HasNext(); it.Next()) {
                visitor(it);
            }
        }
    }

}