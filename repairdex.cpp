//
// Created by xiaobai on 2021/1/9.
//

#include "repairdex.h"

RepairDex::RepairDex(uint8_t *dex_data, size_t dex_len) : BaseRepair(dex_data, dex_len) {

}

void RepairDex::repair() {
    for (const auto &class_data : mHeader->ClassDatas()) {
        for (auto &direct_method : *class_data.get()->DirectMethods()) {
            repairNopCode(direct_method->GetCodeItem());
        }
        for (auto &virtual_method: *class_data.get()->VirtualMethods()) {
            repairNopCode(virtual_method->GetCodeItem());
        }
    }
    for (const auto &methodItem : mHeader->MethodItems()) {
        auto index = methodItem.first;
        auto method_item = methodItem.second;
    }
    for (const auto &codeItem : mHeader->CodeItems()) {

    }
}

/**
 * usage for code item
 * @param codeItem dex code item
 * @return
 */
bool RepairDex::repairNopCode(dex_ir::CodeItem *codeItem) {
    //get this code belong to which method item
    auto method_method = codeItem->GetMethodItem();
    if (codeItem == nullptr)
        return false;

    //get code insns size
    auto code_size = codeItem->InsnsSize();
    //get opcode memory ptr
    uint16_t *code_ptr = codeItem->Insns();
    if (code_size <= 2) {
        return false;
    }
    //get first opcode
    auto opcode = libdex::Instruction::At(code_ptr);
    bool needfix = false;
    if (opcode->Opcode() == libdex::Instruction::RETURN_VOID) {
        needfix = true;
    } else {
        //get next opcode
        opcode++;
    }
    //
    if (opcode->Opcode() == libdex::Instruction::RETURN || opcode->Opcode() == libdex::Instruction::RETURN_OBJECT ||
        opcode->Opcode() == libdex::Instruction::RETURN_WIDE) {
        needfix = true;
    }
    if (needfix) {
        //something .... eg
        //fix opcode
        memcpy(codeItem->Insns(), nullptr, 0);

    }


    return false;
}
