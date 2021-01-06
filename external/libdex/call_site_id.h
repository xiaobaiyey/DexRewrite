//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_CALL_SITE_ID_H
#define BASE_CALL_SITE_ID_H

#include "annotation.h"

namespace dex_ir {
    class CallSiteId : public IndexedItem {
    public:
        explicit CallSiteId(EncodedArrayItem *call_site_item) : call_site_item_(call_site_item) {
            size_ = kCallSiteIdItemSize;
        }

        ~CallSiteId() override {}

        static size_t ItemSize() { return kCallSiteIdItemSize; }

        EncodedArrayItem *CallSiteItem() const { return call_site_item_; }

        //void Accept(AbstractDispatcher* dispatch) const { dispatch->Dispatch(this); }

    private:
        EncodedArrayItem *call_site_item_;

        DISALLOW_COPY_AND_ASSIGN(CallSiteId);
    };
}
#endif //BASE_CALL_SITE_ID_H
