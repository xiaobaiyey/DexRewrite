/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/17 9:31 PM
* @ class describe
*/

#include <inttypes.h>

#include <iomanip>
#include <sstream>
#include <dex/dex_instruction.h>
#include <base/logging.h>
#include <dex/dex_types.h>
#include "dex_utils.h"
#include <dex/dex_file.h>

namespace art {



    const char *const Instruction::kInstructionNames[] = {
#define INSTRUCTION_NAME(o, c, pname, f, i, a, e, v) pname,

#include <dex/dex_instruction_list.h>

            DEX_INSTRUCTION_LIST(INSTRUCTION_NAME)
#undef DEX_INSTRUCTION_LIST
#undef INSTRUCTION_NAME
    };


    static_assert(sizeof(Instruction::InstructionDescriptor) == 8u, "Unexpected descriptor size");

    static constexpr int8_t InstructionSizeInCodeUnitsByOpcode(Instruction::Code opcode,
                                                               Instruction::Format format) {
        if (opcode == Instruction::Code::NOP) {
            return -1;
        } else if ((format >= Instruction::Format::k10x) && (format <= Instruction::Format::k10t)) {
            return 1;
        } else if ((format >= Instruction::Format::k20t) && (format <= Instruction::Format::k22c)) {
            return 2;
        } else if ((format >= Instruction::Format::k32x) && (format <= Instruction::Format::k3rc)) {
            return 3;
        } else if ((format >= Instruction::Format::k45cc) && (format <= Instruction::Format::k4rcc)) {
            return 4;
        } else if (format == Instruction::Format::k51l) {
            return 5;
        } else {
            return -1;
        }
    }

    Instruction::InstructionDescriptor const Instruction::kInstructionDescriptors[] = {
#define INSTRUCTION_DESCR(opcode, c, p, format, index, flags, eflags, vflags) \
    { vflags, \
      format, \
      index, \
      flags, \
      InstructionSizeInCodeUnitsByOpcode((c), (format)), \
    },

#include <dex/dex_instruction_list.h>

            DEX_INSTRUCTION_LIST(INSTRUCTION_DESCR)
#undef DEX_INSTRUCTION_LIST
#undef INSTRUCTION_DESCR
    };

    int32_t Instruction::GetTargetOffset() const {
        switch (FormatOf(Opcode())) {
            // Cases for conditional branches follow.
            case k22t:
                return VRegC_22t();
            case k21t:
                return VRegB_21t();
                // Cases for unconditional branches follow.
            case k10t:
                return VRegA_10t();
            case k20t:
                return VRegA_20t();
            case k30t:
                return VRegA_30t();
            default: LOG(FATAL) << "Tried to access the branch offset of an instruction " << Name() <<
                                " which does not have a target operand.";
              }
        return 0;
    }

    bool Instruction::CanFlowThrough() const {
        const uint16_t *insns = reinterpret_cast<const uint16_t *>(this);
        uint16_t insn = *insns;
        Code opcode = static_cast<Code>(insn & 0xFF);
        return FlagsOf(opcode) & Instruction::kContinue;
    }

    size_t Instruction::SizeInCodeUnitsComplexOpcode() const {
        const uint16_t *insns = reinterpret_cast<const uint16_t *>(this);
        // Handle special NOP encoded variable length sequences.
        switch (*insns) {
            case kPackedSwitchSignature:
                return (4 + insns[1] * 2);
            case kSparseSwitchSignature:
                return (2 + insns[1] * 4);
            case kArrayDataSignature: {
                uint16_t element_size = insns[1];
                uint32_t length = insns[2] | (((uint32_t) insns[3]) << 16);
                // The plus 1 is to round up for odd size and width.
                return (4 + (element_size * length + 1) / 2);
            }
            default:
                if ((*insns & 0xFF) == 0) {
                    return 1;  // NOP.
                } else {
                    LOG(FATAL) << "Unreachable: " << DumpString(nullptr);
                    UNREACHABLE();
                }
        }
    }

    size_t Instruction::CodeUnitsRequiredForSizeOfComplexOpcode() const {
        const uint16_t *insns = reinterpret_cast<const uint16_t *>(this);
        // Handle special NOP encoded variable length sequences.
        switch (*insns) {
            case kPackedSwitchSignature:
                FALLTHROUGH_INTENDED;
            case kSparseSwitchSignature:
                return 2;
            case kArrayDataSignature:
                return 4;
            default:
                if ((*insns & 0xFF) == 0) {
                    return 1;  // NOP.
                } else {
                    LOG(FATAL) << "Unreachable: " << DumpString(nullptr);
                    UNREACHABLE();
                }
        }
    }


    std::string Instruction::DumpHex(size_t code_units) const {
        size_t inst_length = SizeInCodeUnits();
        if (inst_length > code_units) {
            inst_length = code_units;
        }
        std::ostringstream os;
        const uint16_t *insn = reinterpret_cast<const uint16_t *>(this);
        for (size_t i = 0; i < inst_length; i++) {
            os << StringPrintf("0x%04x", insn[i]) << " ";
        }
        for (size_t i = inst_length; i < code_units; i++) {
            os << "       ";
        }
        return os.str();
    }

    std::string Instruction::DumpHexLE(size_t instr_code_units) const {
        size_t inst_length = SizeInCodeUnits();
        if (inst_length > instr_code_units) {
            inst_length = instr_code_units;
        }
        std::ostringstream os;
        const uint16_t *insn = reinterpret_cast<const uint16_t *>(this);
        for (size_t i = 0; i < inst_length; i++) {
            os << StringPrintf("%02x%02x", static_cast<uint8_t>(insn[i] & 0x00FF),
                               static_cast<uint8_t>((insn[i] & 0xFF00) >> 8)) << " ";
        }
        for (size_t i = inst_length; i < instr_code_units; i++) {
            os << "     ";
        }
        return os.str();
    }


    std::string Instruction::DumpString(const DexFile *file) const {
        std::ostringstream os;
        const char *opcode = kInstructionNames[Opcode()];
        switch (FormatOf(Opcode())) {
            case k10x:
                os << opcode;
                break;
            case k12x:
                os << StringPrintf("%s v%d, v%d", opcode, VRegA_12x(), VRegB_12x());
                break;
            case k11n:
                os << StringPrintf("%s v%d, #%+d", opcode, VRegA_11n(), VRegB_11n());
                break;
            case k11x:
                os << StringPrintf("%s v%d", opcode, VRegA_11x());
                break;
            case k10t:
                os << StringPrintf("%s %+d", opcode, VRegA_10t());
                break;
            case k20t:
                os << StringPrintf("%s %+d", opcode, VRegA_20t());
                break;
            case k22x:
                os << StringPrintf("%s v%d, v%d", opcode, VRegA_22x(), VRegB_22x());
                break;
            case k21t:
                os << StringPrintf("%s v%d, %+d", opcode, VRegA_21t(), VRegB_21t());
                break;
            case k21s:
                os << StringPrintf("%s v%d, #%+d", opcode, VRegA_21s(), VRegB_21s());
                break;
            case k21h: {
                // op vAA, #+BBBB0000[00000000]
                if (Opcode() == CONST_HIGH16) {
                    uint32_t value = VRegB_21h() << 16;
                    os << StringPrintf("%s v%d, #int %+d // 0x%x", opcode, VRegA_21h(), value, value);
                } else {
                    uint64_t value = static_cast<uint64_t>(VRegB_21h()) << 48;
                    os << StringPrintf("%s v%d, #long %+" PRId64 " // 0x%" PRIx64, opcode, VRegA_21h(),
                                       value, value);
                }
            }
                break;
            case k21c: {
                switch (Opcode()) {
                    case CONST_STRING:
                        if (file != nullptr) {
                            uint32_t string_idx = VRegB_21c();
                            if (string_idx < file->NumStringIds()) {
                                os << StringPrintf(
                                        "const-string v%d, %s // string@%d",
                                        VRegA_21c(),
                                        PrintableString(file->StringDataByIdx(StringIndex(string_idx))).c_str(),
                                        string_idx);
                            } else {
                                os << StringPrintf("const-string v%d, <<invalid-string-idx-%d>> // string@%d",
                                                   VRegA_21c(),
                                                   string_idx,
                                                   string_idx);
                            }
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case CHECK_CAST:
                    case CONST_CLASS:
                    case NEW_INSTANCE:
                        if (file != nullptr) {
                            TypeIndex type_idx(VRegB_21c());
                            os << opcode << " v" << static_cast<int>(VRegA_21c()) << ", "
                               << file->PrettyType(type_idx) << " // type@" << type_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case SGET:
                    case SGET_WIDE:
                    case SGET_OBJECT:
                    case SGET_BOOLEAN:
                    case SGET_BYTE:
                    case SGET_CHAR:
                    case SGET_SHORT:
                        if (file != nullptr) {
                            uint32_t field_idx = VRegB_21c();
                            os << opcode << "  v" << static_cast<int>(VRegA_21c()) << ", " << file->PrettyField(field_idx, true)
                               << " // field@" << field_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case SPUT:
                    case SPUT_WIDE:
                    case SPUT_OBJECT:
                    case SPUT_BOOLEAN:
                    case SPUT_BYTE:
                    case SPUT_CHAR:
                    case SPUT_SHORT:
                        if (file != nullptr) {
                            uint32_t field_idx = VRegB_21c();
                            os << opcode << " v" << static_cast<int>(VRegA_21c()) << ", " << file->PrettyField(field_idx, true)
                               << " // field@" << field_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    default:
                        os << StringPrintf("%s v%d, thing@%d", opcode, VRegA_21c(), VRegB_21c());
                        break;
                }
                break;
            }
            case k23x:
                os << StringPrintf("%s v%d, v%d, v%d", opcode, VRegA_23x(), VRegB_23x(), VRegC_23x());
                break;
            case k22b:
                os << StringPrintf("%s v%d, v%d, #%+d", opcode, VRegA_22b(), VRegB_22b(), VRegC_22b());
                break;
            case k22t:
                os << StringPrintf("%s v%d, v%d, %+d", opcode, VRegA_22t(), VRegB_22t(), VRegC_22t());
                break;
            case k22s:
                os << StringPrintf("%s v%d, v%d, #%+d", opcode, VRegA_22s(), VRegB_22s(), VRegC_22s());
                break;
            case k22c: {
                switch (Opcode()) {
                    case IGET:
                    case IGET_WIDE:
                    case IGET_OBJECT:
                    case IGET_BOOLEAN:
                    case IGET_BYTE:
                    case IGET_CHAR:
                    case IGET_SHORT:
                        if (file != nullptr) {
                            uint32_t field_idx = VRegC_22c();
                            os << opcode << " v" << static_cast<int>(VRegA_22c()) << ", v" << static_cast<int>(VRegB_22c()) << ", "
                               << file->PrettyField(field_idx, true) << " // field@" << field_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case IGET_QUICK:
                    case IGET_OBJECT_QUICK:
                        if (file != nullptr) {
                            uint32_t field_idx = VRegC_22c();
                            os << opcode << " v" << static_cast<int>(VRegA_22c()) << ", v" << static_cast<int>(VRegB_22c()) << ", "
                               << "// offset@" << field_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case IPUT:
                    case IPUT_WIDE:
                    case IPUT_OBJECT:
                    case IPUT_BOOLEAN:
                    case IPUT_BYTE:
                    case IPUT_CHAR:
                    case IPUT_SHORT:
                        if (file != nullptr) {
                            uint32_t field_idx = VRegC_22c();
                            os << opcode << " v" << static_cast<int>(VRegA_22c()) << ", v" << static_cast<int>(VRegB_22c()) << ", "
                               << file->PrettyField(field_idx, true) << " // field@" << field_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case IPUT_QUICK:
                    case IPUT_OBJECT_QUICK:
                        if (file != nullptr) {
                            uint32_t field_idx = VRegC_22c();
                            os << opcode << " v" << static_cast<int>(VRegA_22c()) << ", v" << static_cast<int>(VRegB_22c()) << ", "
                               << "// offset@" << field_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case INSTANCE_OF:
                        if (file != nullptr) {
                            TypeIndex type_idx(VRegC_22c());
                            os << opcode << " v" << static_cast<int>(VRegA_22c()) << ", v"
                               << static_cast<int>(VRegB_22c()) << ", " << file->PrettyType(type_idx)
                               << " // type@" << type_idx.index_;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case NEW_ARRAY:
                        if (file != nullptr) {
                            TypeIndex type_idx(VRegC_22c());
                            os << opcode << " v" << static_cast<int>(VRegA_22c()) << ", v"
                               << static_cast<int>(VRegB_22c()) << ", " << file->PrettyType(type_idx)
                               << " // type@" << type_idx.index_;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    default:
                        os << StringPrintf("%s v%d, v%d, thing@%d", opcode, VRegA_22c(), VRegB_22c(), VRegC_22c());
                        break;
                }
                break;
            }
            case k32x:
                os << StringPrintf("%s v%d, v%d", opcode, VRegA_32x(), VRegB_32x());
                break;
            case k30t:
                os << StringPrintf("%s %+d", opcode, VRegA_30t());
                break;
            case k31t:
                os << StringPrintf("%s v%d, %+d", opcode, VRegA_31t(), VRegB_31t());
                break;
            case k31i:
                os << StringPrintf("%s v%d, #%+d", opcode, VRegA_31i(), VRegB_31i());
                break;
            case k31c:
                if (Opcode() == CONST_STRING_JUMBO) {
                    uint32_t string_idx = VRegB_31c();
                    if (file != nullptr) {
                        if (string_idx < file->NumStringIds()) {
                            os << StringPrintf(
                                    "%s v%d, %s // string@%d",
                                    opcode,
                                    VRegA_31c(),
                                    PrintableString(file->StringDataByIdx(StringIndex(string_idx))).c_str(),
                                    string_idx);
                        } else {
                            os << StringPrintf("%s v%d, <<invalid-string-idx-%d>> // string@%d",
                                               opcode,
                                               VRegA_31c(),
                                               string_idx,
                                               string_idx);
                        }
                    } else {
                        os << StringPrintf("%s v%d, string@%d", opcode, VRegA_31c(), string_idx);
                    }
                } else {
                    os << StringPrintf("%s v%d, thing@%d", opcode, VRegA_31c(), VRegB_31c());
                    break;
                }
                break;
            case k35c: {
                uint32_t arg[kMaxVarArgRegs];
                GetVarArgs(arg);
                auto DumpArgs = [&](size_t count) {
                    for (size_t i = 0; i < count; ++i) {
                        if (i != 0) {
                            os << ", ";
                        }
                        os << "v" << arg[i];
                    }
                };
                switch (Opcode()) {
                    case FILLED_NEW_ARRAY: {
                        os << opcode << " {";
                        DumpArgs(VRegA_35c());
                        os << "}, type@" << VRegB_35c();
                    }
                        break;

                    case INVOKE_VIRTUAL:
                    case INVOKE_SUPER:
                    case INVOKE_DIRECT:
                    case INVOKE_STATIC:
                    case INVOKE_INTERFACE:
                        if (file != nullptr) {
                            os << opcode << " {";
                            uint32_t method_idx = VRegB_35c();
                            DumpArgs(VRegA_35c());
                            os << "}, " << file->PrettyMethod(method_idx) << " // method@" << method_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case INVOKE_VIRTUAL_QUICK:
                        if (file != nullptr) {
                            os << opcode << " {";
                            uint32_t method_idx = VRegB_35c();
                            DumpArgs(VRegA_35c());
                            os << "},  // vtable@" << method_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case INVOKE_CUSTOM:
                        if (file != nullptr) {
                            os << opcode << " {";
                            uint32_t call_site_idx = VRegB_35c();
                            DumpArgs(VRegA_35c());
                            os << "},  // call_site@" << call_site_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    default:
                        os << opcode << " {";
                        DumpArgs(VRegA_35c());
                        os << "}, thing@" << VRegB_35c();
                        break;
                }
                break;
            }
            case k3rc: {
                uint16_t first_reg = VRegC_3rc();
                uint16_t last_reg = VRegC_3rc() + VRegA_3rc() - 1;
                switch (Opcode()) {
                    case INVOKE_VIRTUAL_RANGE:
                    case INVOKE_SUPER_RANGE:
                    case INVOKE_DIRECT_RANGE:
                    case INVOKE_STATIC_RANGE:
                    case INVOKE_INTERFACE_RANGE:
                        if (file != nullptr) {
                            uint32_t method_idx = VRegB_3rc();
                            os << StringPrintf("%s, {v%d .. v%d}, ", opcode, first_reg, last_reg)
                               << file->PrettyMethod(method_idx) << " // method@" << method_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case INVOKE_VIRTUAL_RANGE_QUICK:
                        if (file != nullptr) {
                            uint32_t method_idx = VRegB_3rc();
                            os << StringPrintf("%s, {v%d .. v%d}, ", opcode, first_reg, last_reg)
                               << "// vtable@" << method_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    case INVOKE_CUSTOM_RANGE:
                        if (file != nullptr) {
                            uint32_t call_site_idx = VRegB_3rc();
                            os << StringPrintf("%s, {v%d .. v%d}, ", opcode, first_reg, last_reg)
                               << "// call_site@" << call_site_idx;
                            break;
                        }
                        FALLTHROUGH_INTENDED;
                    default:
                        os << StringPrintf("%s, {v%d .. v%d}, ", opcode, first_reg, last_reg)
                           << "thing@" << VRegB_3rc();
                        break;
                }
                break;
            }
            case k45cc: {
                uint32_t arg[kMaxVarArgRegs];
                GetVarArgs(arg);
                uint32_t method_idx = VRegB_45cc();
                uint32_t proto_idx = VRegH_45cc();
                os << opcode << " {";
                for (int i = 0; i < VRegA_45cc(); ++i) {
                    if (i != 0) {
                        os << ", ";
                    }
                    os << "v" << arg[i];
                }
                os << "}";
                if (file != nullptr) {
                    os << ", " << file->PrettyMethod(method_idx) << ", " << file->GetShorty(proto_idx)
                       << " // ";
                } else {
                    os << ", ";
                }
                os << "method@" << method_idx << ", proto@" << proto_idx;
                break;
            }
            case k4rcc:
                switch (Opcode()) {
                    case INVOKE_POLYMORPHIC_RANGE: {
                        if (file != nullptr) {
                            uint32_t method_idx = VRegB_4rcc();
                            uint32_t proto_idx = VRegH_4rcc();
                            os << opcode << ", {v" << VRegC_4rcc() << " .. v" << (VRegC_4rcc() + VRegA_4rcc())
                               << "}, " << file->PrettyMethod(method_idx) << ", " << file->GetShorty(proto_idx)
                               << " // method@" << method_idx << ", proto@" << proto_idx;
                            break;
                        }
                    }
                        FALLTHROUGH_INTENDED;
                    default: {
                        uint32_t method_idx = VRegB_4rcc();
                        uint32_t proto_idx = VRegH_4rcc();
                        os << opcode << ", {v" << VRegC_4rcc() << " .. v" << (VRegC_4rcc() + VRegA_4rcc())
                           << "}, method@" << method_idx << ", proto@" << proto_idx;
                    }
                }
                break;
            case k51l:
                os << StringPrintf("%s v%d, #%+" PRId64, opcode, VRegA_51l(), VRegB_51l());
                break;
        }
        return os.str();
    }


// Add some checks that ensure the flags make sense. We need a subclass to be in the context of
// Instruction. Otherwise the flags from the instruction list don't work.
    struct InstructionStaticAsserts : private Instruction {
#define IMPLIES(a, b) (!(a) || (b))

#define VAR_ARGS_CHECK(o, c, pname, f, i, a, e, v) \
    static_assert(IMPLIES((f) == k35c || (f) == k45cc, \
                          ((v) & (kVerifyVarArg | kVerifyVarArgNonZero)) != 0), \
                  "Missing var-arg verification");

#include <dex/dex_instruction_list.h>

        DEX_INSTRUCTION_LIST(VAR_ARGS_CHECK)
#undef DEX_INSTRUCTION_LIST
#undef VAR_ARGS_CHECK

#define VAR_ARGS_RANGE_CHECK(o, c, pname, f, i, a, e, v) \
    static_assert(IMPLIES((f) == k3rc || (f) == k4rcc, \
                          ((v) & (kVerifyVarArgRange | kVerifyVarArgRangeNonZero)) != 0), \
                  "Missing var-arg verification");

#include <dex/dex_instruction_list.h>

        DEX_INSTRUCTION_LIST(VAR_ARGS_RANGE_CHECK)
#undef DEX_INSTRUCTION_LIST
#undef VAR_ARGS_RANGE_CHECK

#define EXPERIMENTAL_CHECK(o, c, pname, f, i, a, e, v) \
    static_assert(kHaveExperimentalInstructions || (((a) & kExperimental) == 0), \
                  "Unexpected experimental instruction.");

#include <dex/dex_instruction_list.h>

        DEX_INSTRUCTION_LIST(EXPERIMENTAL_CHECK)
#undef DEX_INSTRUCTION_LIST
#undef EXPERIMENTAL_CHECK
    };

    std::ostream& operator<<(std::ostream& os, const Instruction::Code& code) {
        return os << Instruction::Name(code);
    }

    uint32_t RangeInstructionOperands::GetOperand(size_t operand_index) const {
        DCHECK_LT(operand_index, GetNumberOfOperands());
        return first_operand_ + operand_index;
    }

    uint32_t VarArgsInstructionOperands::GetOperand(size_t operand_index) const {
        DCHECK_LT(operand_index, GetNumberOfOperands());
        return operands_[operand_index];
    }

    uint32_t NoReceiverInstructionOperands::GetOperand(size_t operand_index) const {
        DCHECK_LT(GetNumberOfOperands(), inner_->GetNumberOfOperands());
        // The receiver is the first operand and since we're skipping it, we need to
        // add 1 to the operand_index.
        return inner_->GetOperand(operand_index + 1);
    }

    std::ostream &operator<<(std::ostream &os,  const Instruction::Format & format) {
        std::stringstream ss;
        std::string str;
        ss<<format;
        ss>>str;
        return os <<str.c_str();
    }

//------------------------------------------------------------------------------
// VRegA
//------------------------------------------------------------------------------
    bool Instruction::HasVRegA() const {
        switch (FormatOf(Opcode())) {
            case k10t:
                return true;
            case k10x:
                return true;
            case k11n:
                return true;
            case k11x:
                return true;
            case k12x:
                return true;
            case k20t:
                return true;
            case k21c:
                return true;
            case k21h:
                return true;
            case k21s:
                return true;
            case k21t:
                return true;
            case k22b:
                return true;
            case k22c:
                return true;
            case k22s:
                return true;
            case k22t:
                return true;
            case k22x:
                return true;
            case k23x:
                return true;
            case k30t:
                return true;
            case k31c:
                return true;
            case k31i:
                return true;
            case k31t:
                return true;
            case k32x:
                return true;
            case k35c:
                return true;
            case k3rc:
                return true;
            case k45cc:
                return true;
            case k4rcc:
                return true;
            case k51l:
                return true;
            default:
                return false;
        }
    }

    int32_t Instruction::VRegA() const {
        switch (FormatOf(Opcode())) {
            case k10t:
                return VRegA_10t();
            case k10x:
                return VRegA_10x();
            case k11n:
                return VRegA_11n();
            case k11x:
                return VRegA_11x();
            case k12x:
                return VRegA_12x();
            case k20t:
                return VRegA_20t();
            case k21c:
                return VRegA_21c();
            case k21h:
                return VRegA_21h();
            case k21s:
                return VRegA_21s();
            case k21t:
                return VRegA_21t();
            case k22b:
                return VRegA_22b();
            case k22c:
                return VRegA_22c();
            case k22s:
                return VRegA_22s();
            case k22t:
                return VRegA_22t();
            case k22x:
                return VRegA_22x();
            case k23x:
                return VRegA_23x();
            case k30t:
                return VRegA_30t();
            case k31c:
                return VRegA_31c();
            case k31i:
                return VRegA_31i();
            case k31t:
                return VRegA_31t();
            case k32x:
                return VRegA_32x();
            case k35c:
                return VRegA_35c();
            case k3rc:
                return VRegA_3rc();
            case k45cc:
                return VRegA_45cc();
            case k4rcc:
                return VRegA_4rcc();
            case k51l:
                return VRegA_51l();
            default:
                LOG(FATAL) << "Tried to access vA of instruction " << Name() << " which has no A operand.";
                exit(EXIT_FAILURE);
        }
    }

    int8_t Instruction::VRegA_10t(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k10t);
        return static_cast<int8_t>(InstAA(inst_data));
    }

    uint8_t Instruction::VRegA_10x(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k10x);
        return InstAA(inst_data);
    }

    uint4_t Instruction::VRegA_11n(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k11n);
        return InstA(inst_data);
    }

    uint8_t Instruction::VRegA_11x(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k11x);
        return InstAA(inst_data);
    }

    uint4_t Instruction::VRegA_12x(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k12x);
        return InstA(inst_data);
    }

    int16_t Instruction::VRegA_20t() const {
        //DCHECK_EQ(FormatOf(Opcode()), k20t);
        return static_cast<int16_t>(Fetch16(1));
    }

    uint8_t Instruction::VRegA_21c(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k21c);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_21h(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k21h);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_21s(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k21s);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_21t(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k21t);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_22b(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22b);
        return InstAA(inst_data);
    }

    uint4_t Instruction::VRegA_22c(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22c);
        return InstA(inst_data);
    }

    uint4_t Instruction::VRegA_22s(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22s);
        return InstA(inst_data);
    }

    uint4_t Instruction::VRegA_22t(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22t);
        return InstA(inst_data);
    }

    uint8_t Instruction::VRegA_22x(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22x);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_23x(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k23x);
        return InstAA(inst_data);
    }

    int32_t Instruction::VRegA_30t() const {
        //CHECK_EQ(FormatOf(Opcode()), k30t);
        return static_cast<int32_t>(Fetch32(1));
    }

    uint8_t Instruction::VRegA_31c(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k31c);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_31i(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k31i);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_31t(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k31t);
        return InstAA(inst_data);
    }

    uint16_t Instruction::VRegA_32x() const {
        DCHECK_EQ(FormatOf(Opcode()), k32x);
        return Fetch16(1);
    }

    uint4_t Instruction::VRegA_35c(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k35c);
        return InstB(inst_data);  // This is labeled A in the spec.
    }

    uint8_t Instruction::VRegA_3rc(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k3rc);
        return InstAA(inst_data);
    }

    uint8_t Instruction::VRegA_51l(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k51l);
        return InstAA(inst_data);
    }

    uint4_t Instruction::VRegA_45cc(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k45cc);
        return InstB(inst_data);  // This is labeled A in the spec.
    }

    uint8_t Instruction::VRegA_4rcc(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k4rcc);
        return InstAA(inst_data);
    }

//------------------------------------------------------------------------------
// VRegB
//------------------------------------------------------------------------------
    bool Instruction::HasVRegB() const {
        switch (FormatOf(Opcode())) {
            case k11n:
                return true;
            case k12x:
                return true;
            case k21c:
                return true;
            case k21h:
                return true;
            case k21s:
                return true;
            case k21t:
                return true;
            case k22b:
                return true;
            case k22c:
                return true;
            case k22s:
                return true;
            case k22t:
                return true;
            case k22x:
                return true;
            case k23x:
                return true;
            case k31c:
                return true;
            case k31i:
                return true;
            case k31t:
                return true;
            case k32x:
                return true;
            case k35c:
                return true;
            case k3rc:
                return true;
            case k45cc:
                return true;
            case k4rcc:
                return true;
            case k51l:
                return true;
            default:
                return false;
        }
    }

    bool Instruction::HasWideVRegB() const {
        return FormatOf(Opcode()) == k51l;
    }

    int32_t Instruction::VRegB() const {
        switch (FormatOf(Opcode())) {
            case k11n:
                return VRegB_11n();
            case k12x:
                return VRegB_12x();
            case k21c:
                return VRegB_21c();
            case k21h:
                return VRegB_21h();
            case k21s:
                return VRegB_21s();
            case k21t:
                return VRegB_21t();
            case k22b:
                return VRegB_22b();
            case k22c:
                return VRegB_22c();
            case k22s:
                return VRegB_22s();
            case k22t:
                return VRegB_22t();
            case k22x:
                return VRegB_22x();
            case k23x:
                return VRegB_23x();
            case k31c:
                return VRegB_31c();
            case k31i:
                return VRegB_31i();
            case k31t:
                return VRegB_31t();
            case k32x:
                return VRegB_32x();
            case k35c:
                return VRegB_35c();
            case k3rc:
                return VRegB_3rc();
            case k45cc:
                return VRegB_45cc();
            case k4rcc:
                return VRegB_4rcc();
            case k51l:
                return VRegB_51l();
            default:
                LOG(FATAL) << "Tried to access vB of instruction " << Name() << " which has no B operand.";
                exit(EXIT_FAILURE);
        }
    }

    uint64_t Instruction::WideVRegB() const {
        return VRegB_51l();
    }

    int4_t Instruction::VRegB_11n(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k11n);
        return static_cast<int4_t>((InstB(inst_data) << 28) >> 28);
    }

    uint4_t Instruction::VRegB_12x(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k12x);
        return InstB(inst_data);
    }

    uint16_t Instruction::VRegB_21c() const {
        DCHECK_EQ(FormatOf(Opcode()), k21c);
        return Fetch16(1);
    }

    uint16_t Instruction::VRegB_21h() const {
        DCHECK_EQ(FormatOf(Opcode()), k21h);
        return Fetch16(1);
    }

    int16_t Instruction::VRegB_21s() const {
        DCHECK_EQ(FormatOf(Opcode()), k21s);
        return static_cast<int16_t>(Fetch16(1));
    }

    int16_t Instruction::VRegB_21t() const {
        //DCHECK_EQ(FormatOf(Opcode()), k21t);
        return static_cast<int16_t>(Fetch16(1));
    }

    uint8_t Instruction::VRegB_22b() const {
        DCHECK_EQ(FormatOf(Opcode()), k22b);
        return static_cast<uint8_t>(Fetch16(1) & 0xff);
    }

    uint4_t Instruction::VRegB_22c(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22c);
        return InstB(inst_data);
    }

    uint4_t Instruction::VRegB_22s(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22s);
        return InstB(inst_data);
    }

    uint4_t Instruction::VRegB_22t(uint16_t inst_data) const {
        DCHECK_EQ(FormatOf(Opcode()), k22t);
        return InstB(inst_data);
    }

    uint16_t Instruction::VRegB_22x() const {
        DCHECK_EQ(FormatOf(Opcode()), k22x);
        return Fetch16(1);
    }

    uint8_t Instruction::VRegB_23x() const {
        DCHECK_EQ(FormatOf(Opcode()), k23x);
        return static_cast<uint8_t>(Fetch16(1) & 0xff);
    }

    uint32_t Instruction::VRegB_31c() const {
        DCHECK_EQ(FormatOf(Opcode()), k31c);
        return Fetch32(1);
    }

    int32_t Instruction::VRegB_31i() const {
        DCHECK_EQ(FormatOf(Opcode()), k31i);
        return static_cast<int32_t>(Fetch32(1));
    }

    int32_t Instruction::VRegB_31t() const {
        DCHECK_EQ(FormatOf(Opcode()), k31t);
        return static_cast<int32_t>(Fetch32(1));
    }

    uint16_t Instruction::VRegB_32x() const {
        DCHECK_EQ(FormatOf(Opcode()), k32x);
        return Fetch16(2);
    }

    uint16_t Instruction::VRegB_35c() const {
        DCHECK_EQ(FormatOf(Opcode()), k35c);
        return Fetch16(1);
    }

    uint16_t Instruction::VRegB_3rc() const {
        DCHECK_EQ(FormatOf(Opcode()), k3rc);
        return Fetch16(1);
    }

    uint16_t Instruction::VRegB_45cc() const {
        DCHECK_EQ(FormatOf(Opcode()), k45cc);
        return Fetch16(1);
    }

    uint16_t Instruction::VRegB_4rcc() const {
        DCHECK_EQ(FormatOf(Opcode()), k4rcc);
        return Fetch16(1);
    }

    uint64_t Instruction::VRegB_51l() const {
        DCHECK_EQ(FormatOf(Opcode()), k51l);
        uint64_t vB_wide = Fetch32(1) | ((uint64_t) Fetch32(3) << 32);
        return vB_wide;
    }

//------------------------------------------------------------------------------
// VRegC
//------------------------------------------------------------------------------
    bool Instruction::HasVRegC() const {
        switch (FormatOf(Opcode())) {
            case k22b:
                return true;
            case k22c:
                return true;
            case k22s:
                return true;
            case k22t:
                return true;
            case k23x:
                return true;
            case k35c:
                return true;
            case k3rc:
                return true;
            case k45cc:
                return true;
            case k4rcc:
                return true;
            default:
                return false;
        }
    }

    int32_t Instruction::VRegC() const {
        switch (FormatOf(Opcode())) {
            case k22b:
                return VRegC_22b();
            case k22c:
                return VRegC_22c();
            case k22s:
                return VRegC_22s();
            case k22t:
                return VRegC_22t();
            case k23x:
                return VRegC_23x();
            case k35c:
                return VRegC_35c();
            case k3rc:
                return VRegC_3rc();
            case k45cc:
                return VRegC_45cc();
            case k4rcc:
                return VRegC_4rcc();
            default:
                LOG(FATAL) << "Tried to access vC of instruction " << Name() << " which has no C operand.";
                exit(EXIT_FAILURE);
        }
    }

    int8_t Instruction::VRegC_22b() const {
        DCHECK_EQ(FormatOf(Opcode()), k22b);
        return static_cast<int8_t>(Fetch16(1) >> 8);
    }

    uint16_t Instruction::VRegC_22c() const {
        DCHECK_EQ(FormatOf(Opcode()), k22c);
        return Fetch16(1);
    }

    int16_t Instruction::VRegC_22s() const {
        DCHECK_EQ(FormatOf(Opcode()), k22s);
        return static_cast<int16_t>(Fetch16(1));
    }

    int16_t Instruction::VRegC_22t() const {
        //DCHECK_EQ(FormatOf(Opcode()), k22t);
        return static_cast<int16_t>(Fetch16(1));
    }

    uint8_t Instruction::VRegC_23x() const {
        DCHECK_EQ(FormatOf(Opcode()), k23x);
        return static_cast<uint8_t>(Fetch16(1) >> 8);
    }

    uint4_t Instruction::VRegC_35c() const {
        DCHECK_EQ(FormatOf(Opcode()), k35c);
        return static_cast<uint4_t>(Fetch16(2) & 0x0f);
    }

    uint16_t Instruction::VRegC_3rc() const {
        DCHECK_EQ(FormatOf(Opcode()), k3rc);
        return Fetch16(2);
    }

    uint4_t Instruction::VRegC_45cc() const {
        DCHECK_EQ(FormatOf(Opcode()), k45cc);
        return static_cast<uint4_t>(Fetch16(2) & 0x0f);
    }

    uint16_t Instruction::VRegC_4rcc() const {
        DCHECK_EQ(FormatOf(Opcode()), k4rcc);
        return Fetch16(2);
    }

//------------------------------------------------------------------------------
// VRegH
//------------------------------------------------------------------------------
    bool Instruction::HasVRegH() const {
        switch (FormatOf(Opcode())) {
            case k45cc:
                return true;
            case k4rcc:
                return true;
            default :
                return false;
        }
    }

    int32_t Instruction::VRegH() const {
        switch (FormatOf(Opcode())) {
            case k45cc:
                return VRegH_45cc();
            case k4rcc:
                return VRegH_4rcc();
            default :
                LOG(FATAL) << "Tried to access vH of instruction " << Name() << " which has no H operand.";
                exit(EXIT_FAILURE);
        }
    }

    uint16_t Instruction::VRegH_45cc() const {
        DCHECK_EQ(FormatOf(Opcode()), k45cc);
        return Fetch16(3);
    }

    uint16_t Instruction::VRegH_4rcc() const {
        DCHECK_EQ(FormatOf(Opcode()), k4rcc);
        return Fetch16(3);
    }

    bool Instruction::HasVarArgs() const {
        return (FormatOf(Opcode()) == k35c) || (FormatOf(Opcode()) == k45cc);
    }

    void Instruction::GetVarArgs(uint32_t arg[kMaxVarArgRegs], uint16_t inst_data) const {
        DCHECK(HasVarArgs());

        /*
         * Note that the fields mentioned in the spec don't appear in
         * their "usual" positions here compared to most formats. This
         * was done so that the field names for the argument count and
         * reference index match between this format and the corresponding
         * range formats (3rc and friends).
         *
         * Bottom line: The argument count is always in vA, and the
         * method constant (or equivalent) is always in vB.
         */
        uint16_t regList = Fetch16(2);
        uint4_t count = InstB(inst_data);  // This is labeled A in the spec.
        DCHECK_LE(count, 5U);

        /*
         * Copy the argument registers into the arg[] array, and
         * also copy the first argument (if any) into vC. (The
         * DecodedInstruction structure doesn't have separate
         * fields for {vD, vE, vF, vG}, so there's no need to make
         * copies of those.) Note that cases 5..2 fall through.
         */
        switch (count) {
            case 5:
                arg[4] = InstA(inst_data);
                FALLTHROUGH_INTENDED;
            case 4:
                arg[3] = (regList >> 12) & 0x0f;
                FALLTHROUGH_INTENDED;
            case 3:
                arg[2] = (regList >> 8) & 0x0f;
                FALLTHROUGH_INTENDED;
            case 2:
                arg[1] = (regList >> 4) & 0x0f;
                FALLTHROUGH_INTENDED;
            case 1:
                arg[0] = regList & 0x0f;
                break;
            default:  // case 0
                break;  // Valid, but no need to do anything.
        }
    }

}