//
// Created by xiaob on 2020/1/3.
//

#include "decompile_method.h"

#include <inttypes.h>
#include "builder_maps.h"
#include "libbase/stringprintf.h"
#include "libdex/dex/dex_file.h"
#include "libdex/dex/dex_instruction-inl.h"

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif
#endif

#ifndef PRIx64
#define PRIx64 "lx"
#endif

/*
 * Gets 2 little-endian bytes.
 */
static inline uint16_t Get2LE(unsigned char const *src) {
    return src[0] | (src[1] << 8);
}


/*
 * Helper for dumpInstruction(), which builds the string
 * representation for the index in the given instruction.
 * Returns a pointer to a buffer of sufficient size.
 */
static std::unique_ptr<char[]> IndexString(dex_ir::Header *header,
                                           const libdex::Instruction *dec_insn,
                                           size_t buf_size, bool &out_of_array) {
    std::unique_ptr<char[]> buf(new char[buf_size]);
    // Determine index and width of the string.
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

    // Determine index type.
    size_t outSize = 0;
    switch (libdex::Instruction::IndexTypeOf(dec_insn->Opcode())) {
        case libdex::Instruction::kIndexUnknown:
            // This function should never get called for this type, but do
            // something sensible here, just to help with debugging.
            outSize = snprintf(buf.get(), buf_size, "<unknown-index>");
            out_of_array = true;
            break;
        case libdex::Instruction::kIndexNone:
            // This function should never get called for this type, but do
            // something sensible here, just to help with debugging.
            outSize = snprintf(buf.get(), buf_size, "<no-index>");
            break;
        case libdex::Instruction::kIndexTypeRef:
            if (index < header->TypeIds().Size()) {
                const char *tp = header->TypeIds()[index]->GetStringId()->Data();
                outSize = snprintf(buf.get(), buf_size, "%s // type@%0*x", tp, width, index);
            } else {
                outSize = snprintf(buf.get(), buf_size, "<type?> // type@%0*x", width, index);
                out_of_array = true;
            }
            break;
        case libdex::Instruction::kIndexStringRef:
            if (index < header->StringIds().Size()) {
                const char *st = header->StringIds()[index]->Data();
                outSize = snprintf(buf.get(), buf_size, "\"%s\" // string@%0*x", st, width, index);
            } else {
                outSize = snprintf(buf.get(), buf_size, "<string?> // string@%0*x", width, index);
                out_of_array = true;
            }
            break;
        case libdex::Instruction::kIndexMethodRef:
            if (index < header->MethodIds().Size()) {
                dex_ir::MethodId *method_id = header->MethodIds()[index];
                const char *name = method_id->Name()->Data();
                std::string type_descriptor = method_id->Proto()->GetSignatureForProtoId();
                const char *back_descriptor = method_id->Class()->GetStringId()->Data();
                outSize = snprintf(buf.get(), buf_size, "%s->%s:%s // method@%0*x",
                                   back_descriptor, name, type_descriptor.c_str(), width, index);
            } else {
                outSize = snprintf(buf.get(), buf_size, "<method?> // method@%0*x", width, index);
                out_of_array = true;
            }
            break;
        case libdex::Instruction::kIndexFieldRef:
            if (index < header->FieldIds().Size()) {
                dex_ir::FieldId *field_id = header->FieldIds()[index];
                const char *name = field_id->Name()->Data();
                const char *type_descriptor = field_id->Type()->GetStringId()->Data();
                const char *back_descriptor = field_id->Class()->GetStringId()->Data();
                outSize = snprintf(buf.get(), buf_size, "%s->%s:%s // field@%0*x",
                                   back_descriptor, name, type_descriptor, width, index);
            } else {
                outSize = snprintf(buf.get(), buf_size, "<field?> // field@%0*x", width, index);
                out_of_array = true;
            }
            break;
        case libdex::Instruction::kIndexVtableOffset:
            outSize = snprintf(buf.get(), buf_size, "[%0*x] // vtable #%0*x",
                               width, index, width, index);
            break;
        case libdex::Instruction::kIndexFieldOffset:
            outSize = snprintf(buf.get(), buf_size, "[obj+%0*x]", width, index);
            break;
        case libdex::Instruction::kIndexMethodAndProtoRef: {
            std::string method("<method?>");
            std::string proto("<proto?>");
            if (index < header->MethodIds().Size()) {
                dex_ir::MethodId *method_id = header->MethodIds()[index];
                const char *name = method_id->Name()->Data();
                std::string type_descriptor = (method_id->Proto()->GetSignatureForProtoId());
                const char *back_descriptor = method_id->Class()->GetStringId()->Data();
                method = base::StringPrintf("%s.%s:%s", back_descriptor, name, type_descriptor.c_str());
            } else {
                out_of_array = true;
            }
            if (secondary_index < header->ProtoIds().Size()) {
                dex_ir::ProtoId *proto_id = header->ProtoIds()[secondary_index];
                proto = proto_id->GetSignatureForProtoId();
            } else {
                out_of_array = true;
            }
            outSize = snprintf(buf.get(), buf_size, "%s, %s // method@%0*x, proto@%0*x",
                               method.c_str(), proto.c_str(), width, index, width, secondary_index);
        }
            break;
            // SOME NOT SUPPORTED:
            // case Instruction::kIndexVaries:
            // case Instruction::kIndexInlineMethod:
        default:
            outSize = snprintf(buf.get(), buf_size, "<?>");
            break;
    }  // switch

    // Determine success of string construction.
    if (outSize >= buf_size) {
        // The buffer wasn't big enough; retry with computed size. Note: snprintf()
        // doesn't count/ the '\0' as part of its returned size, so we add explicit
        // space for it here.
        return IndexString(header, dec_insn, outSize + 1, out_of_array);
    }
    return buf;
}

dex_ir::DecompileMethod::DecompileMethod(dex_ir::Header *header, dex_ir::MethodItem *methodItem) : header_(
        header), methodItem_(methodItem) {
    if (header == nullptr) {
        init_error = true;
        LOG(ERROR) << "check your dex_ir::Header==nullptr ?";
        return;
    }
    if (methodItem == nullptr) {
        init_error = true;
        LOG(ERROR) << "check your methodItem==nullptr ?";
        return;
    }
    process();
}

dex_ir::DecompileMethod::DecompileMethod(dex_ir::Header *header, uint32_t raw_id) : header_(header) {

    if (header == nullptr) {
        init_error = true;
        LOG(ERROR) << "check your dex_ir::Header==nullptr ?";
        return;
    }
    auto find = header->MethodItems().find(raw_id);
    if (find == header->MethodItems().end()) {
        init_error = true;
        LOG(ERROR) << "can't find method_item with raw_id:" << raw_id;
        return;
    }
    this->methodItem_ = find->second;
    process();
}

void dex_ir::DecompileMethod::process() {
    if (methodItem_->GetCodeItem() == nullptr) {
        LOG(WARNING) << this->methodItem_->GetClassData()->GetClassDef()->getJavaClassName() << "."
                     << GetMethodName() + ":" << GetSignature() << " maybe abstract or native";
        return;
    }
    for (const libdex::DexInstructionPcPair &inst: methodItem_->GetCodeItem()->Instructions()) {
        const uint32_t insn_width = inst->SizeInCodeUnits();
        if (insn_width == 0) {
            LOG(WARNING) << "GLITCH: zero-width instruction at idx=0x" << std::hex << inst.DexPc();
            break;
        }
        DecompileCode decompileCode;
        decompileCode.instruction = &inst.Inst();
        decompileCode.pc_ = inst.DexPc();
        auto str = processInstruction(methodItem_->GetCodeItem(), inst.DexPc(), insn_width, &inst.Inst(),
                                      decompileCode);
        if (!str.empty()) {
            decompileCode.dump_string = str;
            methodbodylines.push_back(decompileCode);
            methodbodytext.push_back(str);
        }
    }

}

std::string
dex_ir::DecompileMethod::processInstruction(const dex_ir::CodeItem *code, uint32_t insn_idx, uint32_t insn_width,
                                            const libdex::Instruction *dec_insn, DecompileCode &decompileCode) {

    // Dump (part of) raw bytes.
    const uint16_t *insns = code->Insns();


    std::string format_str;
    // Dump pseudo-instruction or opcode.
    if (dec_insn->Opcode() == libdex::Instruction::NOP) {
        const uint16_t instr = Get2LE((const uint8_t *) &insns[insn_idx]);
        if (instr == libdex::Instruction::kPackedSwitchSignature) {
            format_str = base::StringPrintf("|%04x: packed-switch-data (%d units)", insn_idx, insn_width);
        } else if (instr == libdex::Instruction::kSparseSwitchSignature) {
            format_str = base::StringPrintf("|%04x: sparse-switch-data (%d units)", insn_idx, insn_width);
        } else if (instr == libdex::Instruction::kArrayDataSignature) {
            format_str = base::StringPrintf("|%04x: array-data (%d units)", insn_idx, insn_width);
        } else {
            format_str = base::StringPrintf("|%04x: nop // spacer", insn_idx);
        }
    } else {
        format_str = base::StringPrintf("|%04x: %s", insn_idx, dec_insn->Name());
    }
    bool out_of_array = false;
    // Set up additional argument.
    std::unique_ptr<char[]> index_buf;
    if (libdex::Instruction::IndexTypeOf(dec_insn->Opcode()) != libdex::Instruction::kIndexNone) {
        index_buf = IndexString(header_, dec_insn, 200, out_of_array);
    }
    if (out_of_array) {
        LOG(WARNING) << "some index id out of array";
        return "";
    }

    // Dump the instruction.
    //
    // NOTE: pDecInsn->DumpString(pDexFile) differs too much from original.
    //
    switch (libdex::Instruction::FormatOf(dec_insn->Opcode())) {
        case libdex::Instruction::k10x:        // op
            break;
        case libdex::Instruction::k12x:        // op vA, vB
            decompileCode.src_register = dec_insn->VRegB();
            decompileCode.dest_register = dec_insn->VRegA();
            format_str += base::StringPrintf(" v%d, v%d", dec_insn->VRegA(), dec_insn->VRegB());
            break;
        case libdex::Instruction::k11n:        // op vA, #+B
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.op_value = (int32_t) dec_insn->VRegB();
            format_str += base::StringPrintf(" v%d, #int %d // #%x",
                                             dec_insn->VRegA(), (int32_t) dec_insn->VRegB(),
                                             (uint8_t) dec_insn->VRegB());
            break;
        case libdex::Instruction::k11x:        // op vAA
            decompileCode.dest_register = dec_insn->VRegA();
            format_str += base::StringPrintf(" v%d", dec_insn->VRegA());

            break;
        case libdex::Instruction::k10t:        // op +AA
        case libdex::Instruction::k20t: {      // op +AAAA
            const int32_t targ = (int32_t) dec_insn->VRegA();
            decompileCode.op_value = insn_idx + targ;
            format_str += base::StringPrintf(" %04x // %c%04x",
                                             insn_idx + targ,
                                             (targ < 0) ? '-' : '+',
                                             (targ < 0) ? -targ : targ);
            break;
        }
        case libdex::Instruction::k22x:        // op vAA, vBBBB
            decompileCode.src_register = dec_insn->VRegB();
            decompileCode.dest_register = dec_insn->VRegA();
            format_str += base::StringPrintf(" v%d, v%d", dec_insn->VRegA(), dec_insn->VRegB());
            break;
        case libdex::Instruction::k21t: {     // op vAA, +BBBB
            const int32_t targ = (int32_t) dec_insn->VRegB();
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.op_value = insn_idx + targ;
            format_str += base::StringPrintf(" v%d, %04x // %c%04x", dec_insn->VRegA(),
                                             insn_idx + targ,
                                             (targ < 0) ? '-' : '+',
                                             (targ < 0) ? -targ : targ);
            break;
        }
        case libdex::Instruction::k21s:        // op vAA, #+BBBB
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.op_value = (int32_t) dec_insn->VRegB();
            format_str += base::StringPrintf(" v%d, #int %d // #%x",
                                             dec_insn->VRegA(), (int32_t) dec_insn->VRegB(),
                                             (uint16_t) dec_insn->VRegB());
            break;
        case libdex::Instruction::k21h:        // op vAA, #+BBBB0000[00000000]
            // The printed format varies a bit based on the actual opcode.
            if (dec_insn->Opcode() == libdex::Instruction::CONST_HIGH16) {
                const int32_t value = dec_insn->VRegB() << 16;
                decompileCode.dest_register = dec_insn->VRegA();
                decompileCode.op_value = value;
                format_str += base::StringPrintf(" v%d, #int %d // #%x",
                                                 dec_insn->VRegA(), value, (uint16_t) dec_insn->VRegB());
            } else {
                const int64_t value = ((int64_t) dec_insn->VRegB()) << 48;
                decompileCode.dest_register = dec_insn->VRegA();
                decompileCode.op_value = value;
                format_str += base::StringPrintf(" v%d, #long %"
                                                 PRId64
                                                 " // #%x",
                                                 dec_insn->VRegA(), value, (uint16_t) dec_insn->VRegB());
            }
            break;
        case libdex::Instruction::k21c:        // op vAA, thing@BBBB
            switch (dec_insn->Opcode()) {
                case libdex::Instruction::CONST_STRING: {
                    uint32_t string_idx = dec_insn->VRegB_21c();
                    decompileCode.dest_register = dec_insn->VRegA_21c();
                    decompileCode.op_string_idx = string_idx;
                    break;
                }
                case libdex::Instruction::CHECK_CAST:
                case libdex::Instruction::CONST_CLASS:
                case libdex::Instruction::NEW_INSTANCE: {
                    uint32_t type_idx = dec_insn->VRegB_21c();
                    decompileCode.dest_register = dec_insn->VRegA_21c();
                    decompileCode.op_type_idx = type_idx;
                }
                    break;
                case libdex::Instruction::SGET:
                case libdex::Instruction::SGET_WIDE:
                case libdex::Instruction::SGET_OBJECT:
                case libdex::Instruction::SGET_BOOLEAN:
                case libdex::Instruction::SGET_BYTE:
                case libdex::Instruction::SGET_CHAR:
                case libdex::Instruction::SGET_SHORT: {
                    uint32_t field_idx = dec_insn->VRegB_21c();
                    decompileCode.dest_register = dec_insn->VRegA_21c();
                    decompileCode.op_field_idx = field_idx;
                }
                    break;
                case libdex::Instruction::SPUT:
                case libdex::Instruction::SPUT_WIDE:
                case libdex::Instruction::SPUT_OBJECT:
                case libdex::Instruction::SPUT_BOOLEAN:
                case libdex::Instruction::SPUT_BYTE:
                case libdex::Instruction::SPUT_CHAR:
                case libdex::Instruction::SPUT_SHORT: {
                    uint32_t field_idx = dec_insn->VRegB_21c();
                    decompileCode.dest_register = dec_insn->VRegA_21c();
                    decompileCode.op_field_idx = field_idx;
                }
                    break;
                default:
                    break;
            }
            format_str += base::StringPrintf(" v%d, %s", dec_insn->VRegA(), index_buf.get());
            break;
        case libdex::Instruction::k31c: {        // op vAA, thing@BBBBBBBB
            if (dec_insn->Opcode() == libdex::Instruction::CONST_STRING_JUMBO) {
                uint32_t string_idx = dec_insn->VRegB_31c();
                decompileCode.op_string_idx = dec_insn->VRegA_31c();

            } else {
                decompileCode.dest_register = dec_insn->VRegA();
                decompileCode.op_value = dec_insn->VRegB();
            }
            format_str += base::StringPrintf(" v%d, %s", dec_insn->VRegA(), index_buf.get());
        }
            break;
        case libdex::Instruction::k23x:        // op vAA, vBB, vCC
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.src_register = dec_insn->VRegB();
            format_str += base::StringPrintf(" v%d, v%d, v%d",
                                             dec_insn->VRegA(), dec_insn->VRegB(), dec_insn->VRegC());
            break;
        case libdex::Instruction::k22b:        // op vAA, vBB, #+CC
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.src_register = dec_insn->VRegB();
            decompileCode.op_value = (int32_t) dec_insn->VRegC();
            format_str += base::StringPrintf(" v%d, v%d, #int %d // #%02x",
                                             dec_insn->VRegA(), dec_insn->VRegB(),
                                             (int32_t) dec_insn->VRegC(), (uint8_t) dec_insn->VRegC());
            break;
        case libdex::Instruction::k22t: {      // op vA, vB, +CCCC
            const int32_t targ = (int32_t) dec_insn->VRegC();
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.src_register = dec_insn->VRegB();
            decompileCode.op_value = insn_idx + targ;
            format_str += base::StringPrintf(" v%d, v%d, %04x // %c%04x",
                                             dec_insn->VRegA(), dec_insn->VRegB(),
                                             insn_idx + targ,
                                             (targ < 0) ? '-' : '+',
                                             (targ < 0) ? -targ : targ);
            break;
        }
        case libdex::Instruction::k22s:        // op vA, vB, #+CCCC
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.src_register = dec_insn->VRegB();
            decompileCode.op_value = (int32_t) dec_insn->VRegC();
            format_str += base::StringPrintf(" v%d, v%d, #int %d // #%04x",
                                             dec_insn->VRegA(), dec_insn->VRegB(),
                                             (int32_t) dec_insn->VRegC(),
                                             (uint16_t) dec_insn->VRegC());
            break;
        case libdex::Instruction::k22c: {        // op vA, vB, thing@CCCC
            switch (dec_insn->Opcode()) {
                case libdex::Instruction::IGET:
                case libdex::Instruction::IGET_WIDE:
                case libdex::Instruction::IGET_OBJECT:
                case libdex::Instruction::IGET_BOOLEAN:
                case libdex::Instruction::IGET_BYTE:
                case libdex::Instruction::IGET_CHAR:
                case libdex::Instruction::IGET_SHORT:
                case libdex::Instruction::IGET_QUICK:
                case libdex::Instruction::IGET_OBJECT_QUICK:
                case libdex::Instruction::IPUT:
                case libdex::Instruction::IPUT_WIDE:
                case libdex::Instruction::IPUT_OBJECT:
                case libdex::Instruction::IPUT_BOOLEAN:
                case libdex::Instruction::IPUT_BYTE:
                case libdex::Instruction::IPUT_CHAR:
                case libdex::Instruction::IPUT_SHORT:
                case libdex::Instruction::IPUT_QUICK:
                case libdex::Instruction::IPUT_OBJECT_QUICK: {
                    uint32_t field_idx = dec_insn->VRegC_22c();
                    decompileCode.dest_register = dec_insn->VRegA_22c();
                    decompileCode.src_register = dec_insn->VRegB_22c();
                    decompileCode.op_field_idx = field_idx;
                }
                    break;
                case libdex::Instruction::INSTANCE_OF:
                case libdex::Instruction::NEW_ARRAY: {
                    uint32_t type_idx = (dec_insn->VRegC_22c());
                    decompileCode.dest_register = dec_insn->VRegA_22c();
                    decompileCode.src_register = dec_insn->VRegB_22c();
                    decompileCode.op_type_idx = type_idx;
                }
                    break;
                default:
                    break;
            }
            // NOT SUPPORTED:
            // case Instruction::k22cs:    // [opt] op vA, vB, field offset CCCC
            format_str += base::StringPrintf(" v%d, v%d, %s",
                                             dec_insn->VRegA(), dec_insn->VRegB(), index_buf.get());
        }
            break;
        case libdex::Instruction::k30t:
            format_str += base::StringPrintf(" #%08x", dec_insn->VRegA());
            decompileCode.dest_register = dec_insn->VRegA_22c();
            break;
        case libdex::Instruction::k31i: {     // op vAA, #+BBBBBBBB
            // This is often, but not always, a float.
            union {
                float f;
                uint32_t i;
            } conv;
            conv.i = dec_insn->VRegB();
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.op_value = dec_insn->VRegB();
            format_str += base::StringPrintf(" v%d, #float %g // #%08x",
                                             dec_insn->VRegA(), conv.f, dec_insn->VRegB());
            break;
        }
        case libdex::Instruction::k31t:       // op vAA, offset +BBBBBBBB
            decompileCode.dest_register = dec_insn->VRegA_31t();
            decompileCode.op_value = dec_insn->VRegB_31t();
            format_str += base::StringPrintf(" v%d, %08x // +%08x",
                                             dec_insn->VRegA(), insn_idx + dec_insn->VRegB(),
                                             dec_insn->VRegB());
            break;
        case libdex::Instruction::k32x:        // op vAAAA, vBBBB
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.op_value = dec_insn->VRegB();
            format_str += base::StringPrintf(" v%d, v%d", dec_insn->VRegA(), dec_insn->VRegB());
            break;
        case libdex::Instruction::k35c: {           // op {vC, vD, vE, vF, vG}, thing@BBBB{        // op {vC, vD, vE, vF, vG}, meth@BBBB, proto@HHHH
            // NOT SUPPORTED:
            // case Instruction::k35ms:       // [opt] invoke-virtual+super
            // case Instruction::k35mi:       // [opt] inline invoke
            uint32_t arg[libdex::Instruction::kMaxVarArgRegs];
            dec_insn->GetVarArgs(arg);
            switch (dec_insn->Opcode()) {
                case libdex::Instruction::INVOKE_VIRTUAL:
                case libdex::Instruction::INVOKE_SUPER:
                case libdex::Instruction::INVOKE_DIRECT:
                case libdex::Instruction::INVOKE_INTERFACE: {
                    uint32_t method_idx = dec_insn->VRegB_35c();
                    auto total_register = dec_insn->VRegA();
                    decompileCode.total_register_count = total_register;
                    decompileCode.args_count = total_register == 1 ? 0 : total_register - 1;
                    decompileCode.op_method_idx = method_idx;
                    for (int i = 1; i < dec_insn->VRegA(); ++i) {
                        decompileCode.args_registers.push_back(arg[i]);
                    }
                }
                    break;
                case libdex::Instruction::INVOKE_STATIC: {
                    uint32_t method_idx = dec_insn->VRegB_35c();
                    auto total_register = dec_insn->VRegA();
                    decompileCode.total_register_count = total_register;
                    decompileCode.args_count = total_register;
                    decompileCode.op_method_idx = method_idx;
                    for (int i = 0; i < total_register; ++i) {
                        decompileCode.args_registers.push_back(arg[i]);
                    }
                }
                    break;
                default:
                    break;
            }
            format_str += " {";
            for (int i = 0, n = dec_insn->VRegA(); i < n; i++) {
                if (i == 0) {
                    format_str += base::StringPrintf("v%d", arg[i]);
                } else {
                    format_str += base::StringPrintf(", v%d", arg[i]);
                }
            }  // for
            format_str += base::StringPrintf("}, %s", index_buf.get());

        }
            break;
        case libdex::Instruction::k45cc: {
            uint32_t arg[libdex::Instruction::kMaxVarArgRegs];
            dec_insn->GetVarArgs(arg);
            format_str += " {";
            for (int i = 0, n = dec_insn->VRegA(); i < n; i++) {
                if (i == 0) {
                    format_str += base::StringPrintf("v%d", arg[i]);
                } else {
                    format_str += base::StringPrintf(", v%d", arg[i]);
                }
            }  // for
            format_str += base::StringPrintf("}, %s", index_buf.get());
        }
            break;
        case libdex::Instruction::k3rc: {         // op {vCCCC .. v(CCCC+AA-1)}, thing@BBBB
            {
                uint16_t first_reg = dec_insn->VRegC_3rc();
                uint16_t last_reg = dec_insn->VRegC_3rc() + dec_insn->VRegA_3rc() - 1;
                switch (dec_insn->Opcode()) {
                    case libdex::Instruction::INVOKE_VIRTUAL_RANGE:
                    case libdex::Instruction::INVOKE_SUPER_RANGE:
                    case libdex::Instruction::INVOKE_DIRECT_RANGE:
                    case libdex::Instruction::INVOKE_INTERFACE_RANGE: {
                        uint32_t method_idx = dec_insn->VRegB_3rc();
                        decompileCode.total_register_count = last_reg - first_reg + 1;
                        decompileCode.args_count =
                                decompileCode.total_register_count == 1 ? 0 : decompileCode.total_register_count - 1;
                        decompileCode.op_method_idx = method_idx;
                        for (int i = 1; i < dec_insn->VRegA(); ++i) {
                            decompileCode.args_registers.push_back(dec_insn->VRegC() + i);
                        }

                    }
                        break;
                    case libdex::Instruction::INVOKE_STATIC_RANGE: {
                        uint32_t method_idx = dec_insn->VRegB_3rc();
                        decompileCode.total_register_count = last_reg - first_reg + 1;
                        decompileCode.args_count = decompileCode.total_register_count;
                        decompileCode.op_method_idx = method_idx;
                        for (int i = 0; i < dec_insn->VRegA(); ++i) {
                            decompileCode.args_registers.push_back(dec_insn->VRegC() + i);
                        }
                    }
                        break;
                    default:
                        break;
                }
            }
            format_str += " {";
            for (int i = 0, n = dec_insn->VRegA(); i < n; i++) {
                if (i == 0) {
                    format_str += base::StringPrintf("v%d", dec_insn->VRegC() + i);
                } else {
                    format_str += base::StringPrintf(", v%d", dec_insn->VRegC() + i);
                }
            }  // for
            format_str += base::StringPrintf("}, %s", index_buf.get());
        }
            break;
        case libdex::Instruction::k4rcc:          // op {vCCCC .. v(CCCC+AA-1)}, meth@BBBB, proto@HHHH
            // NOT SUPPORTED:
            // case Instruction::k3rms:       // [opt] invoke-virtual+super/range
            // case Instruction::k3rmi:       // [opt] execute-inline/range
        {
            // This doesn't match the "dx" output when some of the args are
            // 64-bit values -- dx only shows the first register.
            format_str += " {";
            for (int i = 0, n = dec_insn->VRegA(); i < n; i++) {
                if (i == 0) {
                    format_str += base::StringPrintf("v%d", dec_insn->VRegC() + i);
                } else {
                    format_str += base::StringPrintf(", v%d", dec_insn->VRegC() + i);
                }
            }  // for
            format_str += base::StringPrintf("}, %s", index_buf.get());
        }
            break;
        case libdex::Instruction::k51l: {      // op vAA, #+BBBBBBBBBBBBBBBB
            // This is often, but not always, a double.
            union {
                double d;
                uint64_t j;
            } conv;
            conv.j = dec_insn->WideVRegB();
            decompileCode.dest_register = dec_insn->VRegA();
            decompileCode.op_value = dec_insn->WideVRegB();
            format_str += base::StringPrintf(" v%d, #double %g // #%016"
                                             PRIx64, dec_insn->VRegA(), conv.d, dec_insn->WideVRegB());
            break;
        }
            // NOT SUPPORTED:
            // case Instruction::k00x:        // unknown op or breakpoint
            //    break;
        default:
            format_str += " ???";
            break;
    }  // switch
    return format_str;
}


std::string dex_ir::DecompileMethod::GetMethodName() const {
    if (init_error) {
        return "";
    }
    return this->methodItem_->GetMethodName();
}

std::string dex_ir::DecompileMethod::GetSignature() const {
    if (init_error) {
        return "";
    }
    return this->methodItem_->GetSignature();
}

std::string dex_ir::DecompileMethod::GetClassName() const {
    if (init_error) {
        return "";
    }
    return this->methodItem_->GetRawClassName();
}

std::string dex_ir::DecompileMethod::GetJavaClassName() const {
    if (init_error) {
        return "";
    }
    return this->methodItem_->GetClassData()->GetClassDef()->getJavaClassName();
}

bool dex_ir::DecompileMethod::CheckStaus() {
    return !init_error;
}

uint16_t dex_ir::DecompileMethod::RegistersSize() const {
    if (methodItem_->GetCodeItem() == nullptr) {
        return 0;
    }
    return methodItem_->GetCodeItem()->RegistersSize();
}

uint16_t dex_ir::DecompileMethod::InsSize() const {
    if (methodItem_->GetCodeItem() == nullptr) {
        return 0;
    }
    return methodItem_->GetCodeItem()->InsSize();
}

uint16_t dex_ir::DecompileMethod::OutsSize() const {
    if (methodItem_->GetCodeItem() == nullptr) {
        return 0;
    }
    return methodItem_->GetCodeItem()->OutsSize();
}

uint32_t dex_ir::DecompileMethod::InsnsSize() const {
    if (methodItem_->GetCodeItem() == nullptr) {
        return 0;
    }
    return methodItem_->GetCodeItem()->InsnsSize();
}

std::string dex_ir::DecompileMethod::ToString() const {
    if (methodItem_->GetCodeItem() == nullptr) {
        return " ";
    }
    std::string result;
    for (const auto &methodbodyline: methodbodylines) {
        result.append(methodbodyline.dump_string);
        result.append("\r\n");
    }
    return result;
}

std::vector<dex_ir::DecompileCode> dex_ir::DecompileMethod::getMethodBody() {
    if (methodItem_->GetCodeItem() == nullptr) {
        return std::vector<DecompileCode>();
    }
    return this->methodbodylines;
}

uint32_t dex_ir::DecompileMethod::GetIndexId(const libdex::Instruction *dec_insn) {
    uint32_t index = 0;
    uint32_t width = 4;
    switch (libdex::Instruction::FormatOf(dec_insn->Opcode())) {
        case libdex::Instruction::k21c:
        case libdex::Instruction::k35c:
        case libdex::Instruction::k3rc:
            index = dec_insn->VRegB();
            width = 4;
            break;
        case libdex::Instruction::k31c:
            index = dec_insn->VRegB();
            width = 8;
            break;
        case libdex::Instruction::k22c:
            index = dec_insn->VRegC();
            width = 4;
            break;
        case libdex::Instruction::k45cc:
        case libdex::Instruction::k4rcc:
            index = dec_insn->VRegB();
            width = 4;
            break;
        default:
            break;
    }  // switch
    return index;
}

int64_t dex_ir::DecompileMethod::GetRegisterOpValue(uint32_t register_name, uint32_t limit_pc) {
    if (register_name == -1) {
        return -1;
    }
    int64_t result = -1;
    for (const auto &methodbodyline: methodbodylines) {
        if (limit_pc != 0 && methodbodyline.pc_ >= limit_pc) {
            return result;
        }
        if (methodbodyline.dest_register == register_name) {
            result = methodbodyline.op_value;
        }
    }
    return result;
}

std::string dex_ir::DecompileMethod::GetRegisterOpString(uint32_t register_name, uint32_t limit_pc) {
    if (register_name == -1) {
        return std::string();
    }
    uint32_t result_index_id = -1;
    for (const auto &methodbodyline: methodbodylines) {
        if (limit_pc != 0 && methodbodyline.pc_ >= limit_pc) {
            return std::string();
        }
        if (methodbodyline.dest_register == register_name) {
            result_index_id = methodbodyline.op_string_idx;
        }
    }
    if (result_index_id <= this->header_->StringIds().Size()) {
        return this->header_->StringIds()[(result_index_id)]->Data();
    }
    return std::string();
}

const std::vector<std::string> &dex_ir::DecompileMethod::getMethodbodytext() const {
    return methodbodytext;
}
