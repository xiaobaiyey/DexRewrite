//
// Created by xiaob on 2020/1/3.
//

#include "decompilation.h"
#include "libdex/dex/dex_instruction-inl.h"
#include <regex>

namespace dex_ir {

    Decompilation::Decompilation(Header *header) {
        this->header_ = header;
    }

    bool Decompilation::loadReferences() {
        if (indexed) {
            return correct;
        }
        // Iterate over all classes.
        const uint32_t class_defs_size = header_->ClassDefs().Size();
        for (uint32_t i = 0; i < class_defs_size; i++) {
            loadClass(i);
        }  // for
        indexed = true;
        return correct;
    }

    void Decompilation::loadClass(int idx) {
        dex_ir::ClassDef *class_def = header_->ClassDefs()[idx];
        dex_ir::ClassData *class_data = class_def->GetClassData();
        // Direct methods.
        if (class_data != nullptr) {
            dex_ir::MethodItemVector *direct_methods = class_data->DirectMethods();
            if (direct_methods != nullptr) {
                for (uint32_t i = 0; i < direct_methods->size(); i++) {
                    loadMethod((*direct_methods)[i]->GetMethodId()->GetIndex(),
                               (*direct_methods)[i]->GetAccessFlags(),
                               (*direct_methods)[i]->GetCodeItem(),
                               i);
                }  // for
            }
        }// Virtual methods.
        if (class_data != nullptr) {
            dex_ir::MethodItemVector *virtual_methods = class_data->VirtualMethods();
            if (virtual_methods != nullptr) {
                for (uint32_t i = 0; i < virtual_methods->size(); i++) {
                    loadMethod((*virtual_methods)[i]->GetMethodId()->GetIndex(),
                               (*virtual_methods)[i]->GetAccessFlags(),
                               (*virtual_methods)[i]->GetCodeItem(),
                               i);
                }  // for
            }
        }

    }

    void Decompilation::loadMethod(uint32_t idx, uint32_t flags, const dex_ir::CodeItem *code, int i) {
        if (code == nullptr) {
            return;
        }
        loadCode(idx, code);
    }

    void Decompilation::loadCode(uint32_t idx, const dex_ir::CodeItem *code) {
        loadBytecodes(idx, code);
    }

    void Decompilation::loadBytecodes(uint32_t idx, const dex_ir::CodeItem *code) {
        for (const libdex::DexInstructionPcPair &inst: code->Instructions()) {
            const uint32_t insn_width = inst->SizeInCodeUnits();
            if (insn_width == 0) {
                LOG(WARNING) << "GLITCH: zero-width instruction at idx=0x" << std::hex << inst.DexPc();
                break;
            }
            loadInstruction(code, inst.DexPc(), insn_width, &inst.Inst());
        }  // for
    }

    void Decompilation::loadInstruction(const dex_ir::CodeItem *code, uint32_t insn_idx,
                                        uint32_t insn_width, const libdex::Instruction *dec_insn) {
        const uint16_t *insns = code->Insns();
        if (libdex::Instruction::IndexTypeOf(dec_insn->Opcode()) != libdex::Instruction::kIndexNone) {
            loadIndex(code->GetMethodItem(), dec_insn);
        }
    }

    void Decompilation::loadIndex(MethodItem *methodItem, const libdex::Instruction *dec_insn) {
        uint32_t index = 0;
        uint32_t secondary_index = libdex::dex::kDexNoIndex;
        uint32_t width = 4;
        switch (libdex::Instruction::FormatOf(dec_insn->Opcode())) {
            // SOME NOT SUPPORTED:
            // case Instruction::k20bc:
            case libdex::Instruction::k21c:
            case libdex::Instruction::k35c:
                // case Instruction::k35ms:
            case libdex::Instruction::k3rc:
                // case Instruction::k3rms:
                // case Instruction::k35mi:
                // case Instruction::k3rmi:
                index = dec_insn->VRegB();
                width = 4;
                break;
            case libdex::Instruction::k31c:
                index = dec_insn->VRegB();
                width = 8;
                break;
            case libdex::Instruction::k22c:
                // case Instruction::k22cs:
                index = dec_insn->VRegC();
                width = 4;
                break;
            case libdex::Instruction::k45cc:
            case libdex::Instruction::k4rcc:
                index = dec_insn->VRegB();
                secondary_index = dec_insn->VRegH();
                width = 4;
                break;
            default:
                break;
        }  // switch

        switch (libdex::Instruction::IndexTypeOf(dec_insn->Opcode())) {
            case libdex::Instruction::kIndexUnknown:
            case libdex::Instruction::kIndexNone:
                break;
            case libdex::Instruction::kIndexTypeRef:
                if (index < header_->TypeIds().Size()) {
                    auto type_id = header_->TypeIds()[index];
                    //const char *tp = ->GetStringId()->Data();
                    type_id->AddMethodRef(methodItem->GetRawId());
                    methodItem->AddTypeRef(index);
                }
                break;
            case libdex::Instruction::kIndexStringRef:
                if (index < header_->StringIds().Size()) {
                    auto string_id = header_->StringIds()[index];
                    //const char *st = header->GetCollections().GetStringId(index)->Data();
                    string_id->AddMethodRefs(methodItem->GetRawId());
                    methodItem->AddStringRef(index);
                }
                break;
            case libdex::Instruction::kIndexMethodRef:
                if (index < header_->MethodIds().Size()) {
                    auto method_id = header_->MethodIds()[index];
                    /* const char *name = method_id->Name()->Data();
                   std::string type_descriptor = GetSignatureForProtoId(method_id->Proto());
                   const char *back_descriptor = method_id->Class()->GetStringId()->Data();
                   outSize = snprintf(buf.get(), buf_size, "%s.%s:%s // method@%0*x",
                                      back_descriptor, name, type_descriptor.c_str(), width, index);*/
                    method_id->AddMethodRefs(methodItem->GetRawId());
                    methodItem->AddMethodRef(index);
                }
                break;
            case libdex::Instruction::kIndexFieldRef:
                if (index < header_->FieldIds().Size()) {
                    dex_ir::FieldId *field_id = header_->FieldIds()[index];
                    /* const char *name = field_id->Name()->Data();
                     const char *type_descriptor = field_id->Type()->GetStringId()->Data();
                     const char *back_descriptor = field_id->Class()->GetStringId()->Data();
                     outSize = snprintf(buf.get(), buf_size, "%s.%s:%s // field@%0*x",
                                        back_descriptor, name, type_descriptor, width, index);*/
                    field_id->AddMethodRefs(methodItem->GetRawId());
                    methodItem->AddFieldRef(index);
                }
                break;
            case libdex::Instruction::kIndexVtableOffset:
            case libdex::Instruction::kIndexFieldOffset:
            case libdex::Instruction::kIndexMethodAndProtoRef:
                break;
                // SOME NOT SUPPORTED:
                // case Instruction::kIndexVaries:
                // case Instruction::kIndexInlineMethod:
            default:
                break;
        }  // switch

    }

    StringId *Decompilation::getStringIdByValue(std::string value) const {
        for (std::unique_ptr<StringId> &stringId: header_->StringIds()) {
            if (value.compare(stringId->Data()) == 0) {
                return stringId.get();
            }
        }
        return nullptr;
    }

    std::set<StringId *> Decompilation::getStringIdRegex(std::string value_regex) const {
        std::regex regex(value_regex);
        std::set<StringId *> results;
        for (std::unique_ptr<StringId> &stringId: header_->StringIds()) {
            std::string data = stringId->Data();
            if (data.empty())continue;
            if (std::regex_match(data, regex)) {
                results.insert(stringId.get());
            }
        }
        return results;
    }

    std::set<StringId *> Decompilation::getStringIdAllContains(std::string value) const {
        std::set<StringId *> string_ids;
        for (std::unique_ptr<StringId> &stringId: header_->StringIds()) {
            if (stringId->Data() == nullptr)continue;
            std::string temp = stringId->Data();
            if (temp.find(value) != std::string::npos) {
                string_ids.insert(stringId.get());
            }
        }
        return string_ids;
    }

    StringId *Decompilation::getStringIdFirstContains(std::string value) const {
        for (std::unique_ptr<StringId> &stringId: header_->StringIds()) {
            if (stringId->Data() == nullptr)continue;
            std::string temp = stringId->Data();
            if (temp.find(value) != std::string::npos) {
                return stringId.get();
            }
        }
        return nullptr;
    }

    /**
     * find method_id by signature
     * @param class_name The class to which the function belongs eg:Ljava/lang/System;
     * @param method_name function name eg. exit
     * @param signature function signature eg. (I)V
     * @return if find success retrun MethodID pointer @see MethodId
     */
    MethodId *
    Decompilation::getMethodIdBySignatrue(std::string class_name, std::string method_name, std::string signature) {
        for (const auto &method_id: header_->MethodIds()) {
            if (class_name.compare(method_id->Class()->GetStringId()->Data()) != 0) {
                continue;
            }
            if (method_name.compare(method_id->Name()->Data()) != 0) {
                continue;
            }
            if (signature.compare(method_id->Proto()->GetSignatureForProtoId()) != 0) {
                continue;
            }
            return method_id.get();
        }
        return nullptr;
    }

    FieldId *
    Decompilation::getFieldIdBySignatrue(std::string class_name, std::string field_name, std::string signature) {
        for (const auto &field_id: header_->FieldIds()) {
            if (class_name.compare(field_id->Class()->GetStringId()->Data()) != 0) {
                continue;
            }
            if (field_name.compare(field_id->Name()->Data()) != 0) {
                continue;
            }
            if (signature.compare(field_id->Type()->GetStringId()->Data()) != 0) {
                continue;
            }
            return field_id.get();
        }
        return nullptr;
    }

    TypeId *Decompilation::getTypeIdIdBySignatrue(std::string value) const {
        for (const auto &type_id: header_->TypeIds()) {
            if (value == type_id->GetStringId()->Data()) {
                return type_id.get();
            }
        }
        return nullptr;
    }

    std::set<MethodId *> Decompilation::getMethodIdByRegex(std::string class_name_regex, std::string method_name_regex,
                                                           std::string signature_regex_) {
        std::set<MethodId *> method_ids;
        std::regex classe_regex(class_name_regex);
        std::regex method_regex(method_name_regex);
        std::regex signature_regex(signature_regex_);

        for (const auto &method_id: header_->MethodIds()) {
            std::string class_name = method_id->Class()->GetStringId()->Data();
            if (!std::regex_match(class_name, classe_regex)) {
                continue;
            }
            std::string method_name = method_id->Name()->Data();
            if (!std::regex_match(method_name, method_regex)) {
                continue;
            }
            std::string signature = method_id->Proto()->GetSignatureForProtoId();
            if (!std::regex_match(signature, signature_regex)) {
                continue;
            }
            method_ids.insert(method_id.get());
        }
        return method_ids;
    }

    std::set<TypeId *> Decompilation::getTypeIdIdByRegex(std::string value_regex) const {
        std::regex regex(value_regex);
        std::set<TypeId *> results;
        for (const auto &type_id: header_->TypeIds()) {
            std::string data = type_id->GetStringId()->Data();
            if (data.empty())continue;
            if (std::regex_match(data, regex)) {
                results.insert(type_id.get());
            }
        }
        return results;
    }

    std::set<FieldId *> Decompilation::getFieldIdByRegex(std::string class_name_regex, std::string field_name_regex,
                                                         std::string signature_regex_) const {
        std::set<FieldId *> field_ids;
        std::regex classe_regex(class_name_regex);
        std::regex method_regex(field_name_regex);
        std::regex signature_regex(signature_regex_);

        for (const auto &field_id: header_->FieldIds()) {
            std::string class_name = field_id->Class()->GetStringId()->Data();
            if (!std::regex_match(class_name, classe_regex)) {
                continue;
            }
            std::string method_name = field_id->Name()->Data();
            if (!std::regex_match(method_name, method_regex)) {
                continue;
            }
            std::string signature = field_id->Type()->GetStringId()->Data();
            if (!std::regex_match(signature, signature_regex)) {
                continue;
            }
            field_ids.insert(field_id.get());
        }
        return field_ids;
    }


// loadIndexString
}