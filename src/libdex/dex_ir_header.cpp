/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/6 9:35 PM
* @ class describe
*/


#include <dex/dex_ir_header.h>
#include <dex/dex_ir.h>
namespace dex_ir {

    Header::Header(const uint8_t *magic, uint32_t checksum, const uint8_t *signature,
                   uint32_t endian_tag, uint32_t file_size, uint32_t header_size,
                   uint32_t link_size, uint32_t link_offset, uint32_t data_size,
                   uint32_t data_offset, bool support_default_methods) : Item(0, kHeaderItemSize),
                                                                         checksum_(checksum),
                                                                         endian_tag_(
                                                                                 endian_tag),
                                                                         file_size_(file_size),
                                                                         header_size_(
                                                                                 header_size),
                                                                         link_size_(link_size),
                                                                         link_offset_(
                                                                                 link_offset),
                                                                         data_size_(data_size),
                                                                         data_offset_(
                                                                                 data_offset),
                                                                         support_default_methods_(
                                                                                 support_default_methods) {
        memcpy(magic_, magic, sizeof(magic_));
        memcpy(signature_, signature, sizeof(signature_));
    }

    size_t Header::ItemSize() {
        return kHeaderItemSize;
    }

    const uint8_t *Header::Magic() const {
        return magic_;
    }

    uint32_t Header::Checksum() const {
        return checksum_;
    }

    const uint8_t *Header::Signature() const {
        return signature_;
    }

    uint32_t Header::EndianTag() const {
        return endian_tag_;
    }

    uint32_t Header::FileSize() const {
        return file_size_;
    }

    uint32_t Header::HeaderSize() const {
        return header_size_;
    }

    uint32_t Header::LinkSize() const {
        return link_size_;
    }

    uint32_t Header::LinkOffset() const {
        return link_offset_;
    }

    uint32_t Header::DataSize() const {
        return data_size_;
    }

    uint32_t Header::DataOffset() const {
        return data_offset_;
    }

    void Header::SetChecksum(uint32_t new_checksum) {
        checksum_ = new_checksum;
    }

    void Header::SetSignature(const uint8_t *new_signature) {
        memcpy(signature_, new_signature, sizeof(signature_));
    }

    void Header::SetFileSize(uint32_t new_file_size) {
        file_size_ = new_file_size;
    }

    void Header::SetHeaderSize(uint32_t new_header_size) {
        header_size_ = new_header_size;
    }

    void Header::SetLinkSize(uint32_t new_link_size) {
        link_size_ = new_link_size;
    }

    void Header::SetLinkOffset(uint32_t new_link_offset) {
        link_offset_ = new_link_offset;
    }

    void Header::SetDataSize(uint32_t new_data_size) {
        data_size_ = new_data_size;
    }

    void Header::SetDataOffset(uint32_t new_data_offset) {
        data_offset_ = new_data_offset;
    }

    Collections &Header::GetCollections() {
        return collections_;
    }


}