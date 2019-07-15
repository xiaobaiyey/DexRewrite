/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:35 PM
* @ class describe
*/

#ifndef UNPACKER_HEADER_H
#define UNPACKER_HEADER_H


#include "dex_ir_item.h"
#include "dex_ir_collections.h"
namespace dex_ir {

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
               bool support_default_methods);

        ~Header() override {}

        static size_t ItemSize();

        const uint8_t *Magic() const;

        uint32_t Checksum() const;

        const uint8_t *Signature() const;

        uint32_t EndianTag() const;

        uint32_t FileSize() const;

        uint32_t HeaderSize() const;

        uint32_t LinkSize() const;

        uint32_t LinkOffset() const;

        uint32_t DataSize() const;

        uint32_t DataOffset() const;

        void SetChecksum(uint32_t new_checksum);

        void SetSignature(const uint8_t *new_signature);

        void SetFileSize(uint32_t new_file_size);

        void SetHeaderSize(uint32_t new_header_size);

        void SetLinkSize(uint32_t new_link_size);

        void SetLinkOffset(uint32_t new_link_offset);

        void SetDataSize(uint32_t new_data_size);

        void SetDataOffset(uint32_t new_data_offset);

        Collections &GetCollections();


        bool SupportDefaultMethods() const {
            return support_default_methods_;
        }

    private:
        uint8_t magic_[8];
        uint32_t checksum_;
        uint8_t signature_[20];
        uint32_t endian_tag_;
        uint32_t file_size_;
        uint32_t header_size_;
        uint32_t link_size_;
        uint32_t link_offset_;
        uint32_t data_size_;
        uint32_t data_offset_;
        const bool support_default_methods_;

        Collections collections_;

        DISALLOW_COPY_AND_ASSIGN(Header);
    };
}


#endif //UNPACKER_HEADER_H
