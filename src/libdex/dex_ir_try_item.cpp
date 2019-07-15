/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:08 AM
* @ class describe
*/



#include <dex/dex_ir_try_item.h>

namespace dex_ir {

    TryItem::TryItem(uint32_t start_addr, uint16_t insn_count, const CatchHandler *handlers)
            : start_addr_(start_addr), insn_count_(insn_count), handlers_(handlers) {}
}
