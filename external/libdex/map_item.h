//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_MAP_ITEM_H
#define BASE_MAP_ITEM_H

#include "item.h"

namespace dex_ir {
    class MapItem : public Item {
    public:
        //void Accept(AbstractDispatcher* dispatch) { dispatch->Dispatch(this); }

    private:
        DISALLOW_COPY_AND_ASSIGN(MapItem);
    };

}


#endif //BASE_MAP_ITEM_H
