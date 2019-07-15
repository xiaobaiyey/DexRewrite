/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:17 AM
* @ class describe
*/


#include <dex/dex_ir_debuginfo_item.h>

namespace dex_ir {

    DebugInfoItem::DebugInfoItem(uint32_t debug_info_size, uint8_t *debug_info)
            : debug_info_size_(debug_info_size), debug_info_(debug_info) {

    }
}
