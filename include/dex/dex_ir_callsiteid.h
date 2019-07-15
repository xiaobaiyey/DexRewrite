/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:31 AM
* @ class describe
*/

#ifndef UNPACKER_CALLSITEID_H
#define UNPACKER_CALLSITEID_H


#include "dex_ir_item.h"
#include "dex_ir.h"

namespace dex_ir {
    class CallSiteId : public IndexedItem {
    public:
        explicit CallSiteId(EncodedArrayItem *call_site_item);

        ~CallSiteId() override {}

        static size_t ItemSize() { return kCallSiteIdItemSize; }

        EncodedArrayItem *CallSiteItem() const { return call_site_item_; }


    private:
        EncodedArrayItem *call_site_item_;

        DISALLOW_COPY_AND_ASSIGN(CallSiteId);
    };
}


#endif //UNPACKER_CALLSITEID_H
