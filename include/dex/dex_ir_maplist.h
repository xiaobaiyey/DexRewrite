/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/7 12:33 AM
* @ class describe
*/

#ifndef UNPACKER_MAPLIST_H
#define UNPACKER_MAPLIST_H


namespace dex_ir {
    // TODO(sehr): implement MapList.
    class MapList : public Item {
    private:
        DISALLOW_COPY_AND_ASSIGN(MapList);
    };

    class MapItem : public Item {


    private:
        DISALLOW_COPY_AND_ASSIGN(MapItem);
    };
}


#endif //UNPACKER_MAPLIST_H
