//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_MAP_LIST_H
#define BASE_MAP_LIST_H

#include "item.h"

namespace dex_ir {
    // TODO(sehr): implement MapList.
    class MapList : public Item {
    public:
        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        DISALLOW_COPY_AND_ASSIGN(MapList);
    };
}

#endif //BASE_MAP_LIST_H
