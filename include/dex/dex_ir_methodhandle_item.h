/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:32 AM
* @ class describe
*/

#ifndef UNPACKER_METHODHANDLEITEM_H
#define UNPACKER_METHODHANDLEITEM_H


#include "dex_file.h"
#include "dex_ir.h"

namespace dex_ir {
    class MethodHandleItem : public IndexedItem {
    public:
        MethodHandleItem(art::DexFile::MethodHandleType method_handle_type,
                         IndexedItem *field_or_method_id);

        ~MethodHandleItem() override {}

        static size_t ItemSize() { return kMethodHandleItemSize; }

        art::DexFile::MethodHandleType GetMethodHandleType() const { return method_handle_type_; }

        IndexedItem *GetFieldOrMethodId() const { return field_or_method_id_; }


    private:
        art::DexFile::MethodHandleType method_handle_type_;
        IndexedItem *field_or_method_id_;

        DISALLOW_COPY_AND_ASSIGN(MethodHandleItem);
    };

}

#endif //UNPACKER_METHODHANDLEITEM_H
