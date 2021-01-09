//
// Created by xiaobai on 2021/1/9.
//

#ifndef DEXREWRITE_REPAIRDEX_H
#define DEXREWRITE_REPAIRDEX_H

#include "baserepair.h"

class RepairDex : public BaseRepair {
public:
    RepairDex(uint8_t *dex_data, size_t dex_len);

    void repair();


private:
    bool repairNopCode(dex_ir::CodeItem *codeItem);
};


#endif //DEXREWRITE_REPAIRDEX_H
