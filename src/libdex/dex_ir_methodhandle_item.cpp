#include <dex/dex_ir_item.h>
#include <dex/dex_ir_methodhandle_item.h>

/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:32 AM
* @ class describe
*/



dex_ir::MethodHandleItem::MethodHandleItem(art::DexFile::MethodHandleType method_handle_type,
                                           dex_ir::IndexedItem *field_or_method_id)
        : method_handle_type_(method_handle_type),
          field_or_method_id_(field_or_method_id) {
    size_ = kMethodHandleItemSize;
}
