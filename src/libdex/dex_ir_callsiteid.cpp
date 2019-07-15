/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:31 AM
* @ class describe
*/


#include <dex/dex_ir_callsiteid.h>
#include <dex/dex_ir_annotation.h>
#include <dex/dex_ir.h>

namespace dex_ir {

    CallSiteId::CallSiteId(EncodedArrayItem *call_site_item) : call_site_item_(
            call_site_item) {
        size_ = kCallSiteIdItemSize;
    }
}
