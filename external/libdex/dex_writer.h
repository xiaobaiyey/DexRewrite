//
// Created by xiaob on 2020/1/2.
//

#ifndef BASE_DEX_WRITER_H
#define BASE_DEX_WRITER_H

#include <stdint.h>
#include <queue>
#include <string.h>
#include "libbase/leb128.h"
#include "dex/dex_file.h"
#include "libbase/macros.h"
#include "dex_container.h"
#include "annotation.h"
#include "field_item.h"
#include "method_item.h"
#include "collection_base.h"
#include "dexlayout.h"

namespace dex_ir {
    class Header;

    struct MapItem {
        // Not using DexFile::MapItemType since compact dex and standard dex file may have different
        // sections.
        MapItem() = default;

        MapItem(uint32_t type, uint32_t size, size_t offset)
                : type_(type), size_(size), offset_(offset) {}

        // Sort by decreasing order since the priority_queue puts largest elements first.
        bool operator>(const MapItem &other) const {
            return offset_ > other.offset_;
        }

        uint32_t type_ = 0u;
        uint32_t size_ = 0u;
        uint32_t offset_ = 0u;
    };


    class MapItemQueue : public std::priority_queue<MapItem, std::vector<MapItem>, std::greater<MapItem>> {
    public:
        void AddIfNotEmpty(const MapItem &item);
    };


    class DexWriter {
    public:
        static constexpr uint32_t kDataSectionAlignment = sizeof(uint32_t) * 2;
        static constexpr uint32_t kDexSectionWordAlignment = 4;

        // Stream that writes into a dex container section. Do not have two streams pointing to the same
        // backing storage as there may be invalidation of backing storage to resize the section.
        // Random access stream (consider refactoring).
        class Stream {
        public:
            explicit Stream(DexContainer::Section *section) : section_(section) {
                SyncWithSection();
            }

            const uint8_t *Begin() const {
                return data_;
            }

            // Functions are not virtual (yet) for speed.
            size_t Tell() const {
                return position_;
            }

            void Seek(size_t position) {
                position_ = position;
                EnsureStorage(0u);
            }

            // Does not allow overwriting for bug prevention purposes.
            ALWAYS_INLINE size_t

            Write(const void *buffer, size_t length) {
                EnsureStorage(length);
                for (size_t i = 0; i < length; ++i) {
                    DCHECK_EQ(data_[position_ + i], 0u);
                }
                memcpy(&data_[position_], buffer, length);
                position_ += length;
                return length;
            }

            ALWAYS_INLINE size_t

            Overwrite(const void *buffer, size_t length) {
                EnsureStorage(length);
                memcpy(&data_[position_], buffer, length);
                position_ += length;
                return length;
            }

            ALWAYS_INLINE size_t Clear(size_t position, size_t length) {
                EnsureStorage(length);
                memset(&data_[position], 0, length);
                return length;
            }

            ALWAYS_INLINE size_t WriteSleb128(int32_t value) {
                EnsureStorage(8);
                uint8_t *ptr = &data_[position_];
                const size_t len = base::EncodeSignedLeb128(ptr, value) - ptr;
                position_ += len;
                return len;
            }

            ALWAYS_INLINE size_t WriteUleb128(uint32_t value) {
                EnsureStorage(8);
                uint8_t *ptr = &data_[position_];
                const size_t len = base::EncodeUnsignedLeb128(ptr, value) - ptr;
                position_ += len;
                return len;
            }

            ALWAYS_INLINE void AlignTo(const size_t alignment) {
                position_ = base::RoundUp(position_, alignment);
                EnsureStorage(0u);
            }

            ALWAYS_INLINE void Skip(const size_t count) {
                position_ += count;
                EnsureStorage(0u);
            }

            class ScopedSeek {
            public:
                ScopedSeek(Stream *stream, uint32_t offset) : stream_(stream), offset_(stream->Tell()) {
                    stream->Seek(offset);
                }

                ~ScopedSeek() {
                    stream_->Seek(offset_);
                }

            private:
                Stream *const stream_;
                const uint32_t offset_;
            };

        private:
            ALWAYS_INLINE void EnsureStorage(size_t length) {
                size_t end = position_ + length;
                while (UNLIKELY(end > data_size_)) {
                    section_->Resize(data_size_ * 3 / 2 + 1);
                    SyncWithSection();
                }
            }

            void SyncWithSection() {
                data_ = section_->Begin();
                data_size_ = section_->Size();
            }

            // Current position of the stream.
            size_t position_ = 0u;
            DexContainer::Section *const section_ = nullptr;
            // Cached Begin() from the container to provide faster accesses.
            uint8_t *data_ = nullptr;
            // Cached Size from the container to provide faster accesses.
            size_t data_size_ = 0u;
        };

        static inline constexpr uint32_t SectionAlignment(libdex::DexFile::MapItemType type) {
            switch (type) {
                case libdex::DexFile::kDexTypeClassDataItem:
                case libdex::DexFile::kDexTypeStringDataItem:
                case libdex::DexFile::kDexTypeDebugInfoItem:
                case libdex::DexFile::kDexTypeAnnotationItem:
                case libdex::DexFile::kDexTypeEncodedArrayItem:
                    return alignof(uint8_t);

                default:
                    // All other sections are kDexAlignedSection.
                    return DexWriter::kDexSectionWordAlignment;
            }
        }

        class Container : public DexContainer {
        public:
            Section *GetMainSection() override {
                return &main_section_;
            }

            Section *GetDataSection() override {
                return &data_section_;
            }

            bool IsCompactDexContainer() const override {
                return false;
            }

        private:
            VectorSection main_section_;
            VectorSection data_section_;

            friend class CompactDexWriter;
        };

        DexWriter(DexLayout *dex_layout, bool compute_offsets);

        DexWriter(dex_ir::Header *header, bool compute_offsets);

        static bool Output(DexLayout *dex_layout,
                           std::unique_ptr<DexContainer> *container,
                           bool compute_offsets,
                           std::string *error_msg) WARN_UNUSED;

        static bool Output(dex_ir::Header *header,
                           std::string outpath) WARN_UNUSED;


        virtual ~DexWriter() {}
        virtual std::unique_ptr<DexContainer> CreateDexContainer() const;
        virtual bool Write(DexContainer *output, std::string *error_msg);


    protected:


        void WriteEncodedValue(Stream *stream, dex_ir::EncodedValue *encoded_value);

        void WriteEncodedValueHeader(Stream *stream, int8_t value_type, size_t value_arg);

        void WriteEncodedArray(Stream *stream, dex_ir::EncodedValueVector *values);

        void WriteEncodedAnnotation(Stream *stream, dex_ir::EncodedAnnotation *annotation);

        void WriteEncodedFields(Stream *stream, dex_ir::FieldItemVector *fields);

        void WriteEncodedMethods(Stream *stream, dex_ir::MethodItemVector *methods);

        // Header and id section
        virtual void WriteHeader(Stream *stream);

        virtual size_t GetHeaderSize() const;

        // reserve_only means don't write, only reserve space. This is required since the string data
        // offsets must be assigned.
        void WriteStringIds(Stream *stream, bool reserve_only);

        void WriteTypeIds(Stream *stream);

        void WriteProtoIds(Stream *stream, bool reserve_only);

        void WriteFieldIds(Stream *stream);

        void WriteMethodIds(Stream *stream);

        void WriteClassDefs(Stream *stream, bool reserve_only);

        void WriteCallSiteIds(Stream *stream, bool reserve_only);

        void WriteEncodedArrays(Stream *stream);

        void WriteAnnotations(Stream *stream);

        void WriteAnnotationSets(Stream *stream);

        void WriteAnnotationSetRefs(Stream *stream);

        void WriteAnnotationsDirectories(Stream *stream);

        // Data section.
        void WriteDebugInfoItems(Stream *stream);

        void WriteCodeItems(Stream *stream, bool reserve_only);

        void WriteTypeLists(Stream *stream);

        void WriteStringDatas(Stream *stream);

        void WriteClassDatas(Stream *stream);

        void WriteMethodHandles(Stream *stream);

        void WriteHiddenapiClassData(Stream *stream);

        void WriteMapItems(Stream *stream, MapItemQueue *queue);

        void GenerateAndWriteMapItems(Stream *stream);

        virtual void WriteCodeItemPostInstructionData(Stream *stream,
                                                      dex_ir::CodeItem *item,
                                                      bool reserve_only);

        virtual void WriteCodeItem(Stream *stream, dex_ir::CodeItem *item, bool reserve_only);

        virtual void WriteDebugInfoItem(Stream *stream, dex_ir::DebugInfoItem *debug_info);

        virtual void WriteStringData(Stream *stream, dex_ir::StringData *string_data);

        // Process an offset, if compute_offset is set, write into the dex ir item, otherwise read the
        // existing offset and use that for writing.
        void ProcessOffset(Stream *stream, dex_ir::Item *item);

        void ProcessOffset(Stream *stream, dex_ir::CollectionBase *item);

        dex_ir::Header *const header_;
        DexLayout *const dex_layout_;
        bool compute_offsets_;

    private:
        DISALLOW_COPY_AND_ASSIGN(DexWriter);
    };

}


#endif //BASE_DEX_WRITER_H
