#include "library.h"

#include <iostream>
#include <dex/dex_ir_builder.h>
#include <memory>
#include <dex/dex_ir_writer.h>


inline void collect_string(const art::Instruction *dec_insn, dex_ir::Collections &collections,
                           std::vector<dex_ir::StringId *> *string_ids) {
    uint32_t index = 0;

    switch (art::Instruction::FormatOf(dec_insn->Opcode())) {
        // SOME NOT SUPPORTED:
        // case Instruction::k20bc:
        case art::Instruction::k21c:
        case art::Instruction::k35c:
            // case Instruction::k35ms:
        case art::Instruction::k3rc:
            // case Instruction::k3rms:
            // case Instruction::k35mi:
            // case Instruction::k3rmi:
        case art::Instruction::k45cc:
        case art::Instruction::k4rcc:
            index = dec_insn->VRegB();
            break;
        case art::Instruction::k31c:
            index = dec_insn->VRegB();
            break;
        case art::Instruction::k22c:
            // case Instruction::k22cs:
            index = dec_insn->VRegC();
            break;
        default:
            break;
    }  // switch

    switch (art::Instruction::IndexTypeOf(dec_insn->Opcode())) {
        case art::Instruction::kIndexStringRef: {
            const char *data_str = collections.GetStringId(index)->Data();
            std::cout << data_str << std::endl;
            string_ids->push_back(collections.GetStringId(index));
        }
            break;
        default:
            break;
    }  // switch
}

/**
 * get all dex string
 * @param mHeader
 */
void get_dex_string(dex_ir::Header *mHeader) {
    std::vector<dex_ir::StringId *> string_ids;
    for (auto &item : mHeader->GetCollections().CodeItems()) {
        auto instructions = item.get()->Instructions();
        art::SafeDexInstructionIterator it(instructions.begin(), instructions.end());
        for (; !it.IsErrorState() && it < instructions.end(); ++it) {
            // In case the instruction goes past the end of the code item, make sure to not process it.
            art::SafeDexInstructionIterator next = it;
            ++next;
            if (next.IsErrorState()) {
                break;
            }
            collect_string(&it.Inst(), mHeader->GetCollections(), &string_ids);
        }
    }

}


void lookup_all_method(dex_ir::Header *mHeader) {
    for (auto &method_item : mHeader->GetCollections().MethodItems()) {
        //name
        auto method_name = method_item.second->GetMethodId()->Name()->Data();
        auto method_sign = method_item.second->GetSignure();
        auto class_name = method_item.second->GetClassName();

        //dex_code
        auto code_item = method_item.second->GetCodeItem();
        if (code_item != nullptr) {

        }
        //fix 3
        // memcpy(method->GetCodeItem()->Insns(), dexcode_i1, method->GetCodeItem()->InsnsSize() * 2);
        //create new code_item
        //dex_ir::CodeItem *codeItem = mHeader->GetCollections().CreateCodeItem(*mDexFile.get(),reinterpret_cast<uint8_t *>(dexCode),index,code_items->GetMethodItem()->GetMethodId()->GetIndex());
        //code_items->GetMethodItem()->SetCodeItem(codeItem);
        // change AccessFlags to remove native falgs
        // method_item.second->SetAccessFlags()
    }
}

/**
 * look up all dexcode item
 * @param mHeader
 */
void lookup_all_code(dex_ir::Header *mHeader) {
    for (auto &code_item : mHeader->GetCollections().CodeItems()) {

    }
}


void lookup_all_class(dex_ir::Header *mHeader) {
    for (auto &class_item : mHeader->GetCollections().ClassDefs()) {

    }
}


int main(int argc, char **argv) {

    std::unique_ptr<art::DexFile> dexfile(art::DexFile::getDexFile("classes.dex"));

    auto buildheader = dex_ir::DexIrBuilder(*dexfile.get(), false);

    get_dex_string(buildheader);

    lookup_all_method(buildheader);

    lookup_all_code(buildheader);


    // write to file
    std::ostringstream oss;
    oss << "test.dex";
    DexWriter::Output(buildheader, oss.str());

    return 0;

}


