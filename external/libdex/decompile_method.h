//
// Created by xiaob on 2020/1/3.
//

#ifndef BASE_DECOMPILE_METHOD_H
#define BASE_DECOMPILE_METHOD_H

#include <vector>
#include <string>
#include "header.h"

namespace dex_ir {

    class DecompileCode {
    public:
        std::string dump_string;
        const libdex::Instruction *instruction;
        //当前指令的偏移值
        uint32_t pc_ = 0;
        //当前指令所用到的寄存器数量
        uint32_t total_register_count = -1;
        //当前指令用作参数的寄存器个数
        uint32_t args_count = -1;
        //当前指令的参数寄存器
        std::vector<uint32_t> args_registers;
        //当前指令源寄存器
        uint32_t src_register = -1;
        //当前指令目标寄存器
        uint32_t dest_register = -1;
        //立即数
        int64_t op_value = -1;
        uint32_t op_string_idx = -1;
        uint32_t op_field_idx = -1;
        uint32_t op_type_idx = -1;
        uint32_t op_method_idx = -1;

    public:
        bool has_src_register();

        bool has_dest_register();

    };

    class DecompileMethod {
    public:
        DecompileMethod(Header *header, MethodItem *methodItem);

        DecompileMethod(Header *header, uint32_t raw_id/*method_ref_id not method_id*/);

        bool CheckStaus();

    public:
        uint32_t GetIndexId(const libdex::Instruction *instruction);

        /**
         * 获取寄存器opvauel值
         * @param register_name
         * @param limit_pc
         * @return
         */
        int64_t GetRegisterOpValue(uint32_t register_name, uint32_t limit_pc = 0);

        /**
        * 获取寄存器string id值
        * @param register_name
        * @param limit_pc
        * @return
        */
        std::string GetRegisterOpString(uint32_t register_name, uint32_t limit_pc = 0);


    public:

        std::string GetMethodName() const;

        std::string GetSignature() const;

        std::string GetClassName() const;

        std::string GetJavaClassName() const;

        uint16_t RegistersSize() const;

        uint16_t InsSize() const;

        uint16_t OutsSize() const;

        uint32_t InsnsSize() const;

        std::string ToString() const;

        std::vector<DecompileCode> getMethodBody();

        const std::vector<std::string> &getMethodbodytext() const;

    private:
        void process();

        std::string
        processInstruction(const dex_ir::CodeItem *code, uint32_t insn_idx, uint32_t insn_width,
                           const libdex::Instruction *dec_insn, DecompileCode &decompileCode);

    private:
        std::set<uint32_t> string_id_refs;
        std::set<uint32_t> type_id_refs;


        bool init_error = false;
        Header *header_;
        MethodItem *methodItem_;
        std::vector<DecompileCode> methodbodylines;
        std::vector<std::string> methodbodytext;
    };
}


#endif //BASE_DECOMPILE_METHOD_H
