//
// Created by xiaob on 2020/1/3.
//

#ifndef BASE_DECOMPILATION_H
#define BASE_DECOMPILATION_H


#include "header.h"

namespace dex_ir {
    class Decompilation {
    public:
        explicit Decompilation(Header *header);

        ~Decompilation() {
        };

        bool loadReferences();

        StringId *getStringIdByValue(std::string value) const;

        StringId *getStringIdFirstContains(std::string value) const;

        std::set<StringId *> getStringIdAllContains(std::string value) const;

        std::set<StringId *> getStringIdRegex(std::string value_regex) const;


        std::set<MethodId *>
        getMethodIdByRegex(std::string class_name_regex, std::string method_name_regex,
                           std::string signature_regex_);

        MethodId *getMethodIdBySignatrue(std::string class_name, std::string method_name, std::string signature);



        std::set<TypeId *> getTypeIdIdByRegex(std::string value_regex) const;
        TypeId *getTypeIdIdBySignatrue(std::string value) const;

        FieldId *getFieldIdBySignatrue(std::string class_name, std::string field_name, std::string signature);

        std::set<FieldId *> getFieldIdByRegex(std::string class_name_regex, std::string field_name_regex,
                                              std::string signature_regex_) const ;

    private:
        void loadClass(int idx);

        void loadMethod(uint32_t idx, uint32_t flags, const dex_ir::CodeItem *code, int i);

        void loadCode(uint32_t idx, const dex_ir::CodeItem *code);

        void loadBytecodes(uint32_t idx, const dex_ir::CodeItem *code);

        void loadInstruction(const dex_ir::CodeItem *code, uint32_t insn_idx, uint32_t insn_width,
                             const libdex::Instruction *dec_insn);

        void loadIndex(MethodItem *methodItem, const libdex::Instruction *dec_insn);


    private:


    private:
        Header *header_;
        bool indexed = false;
        bool correct = true;
        bool debug_info;
    private:
        DISALLOW_COPY_AND_ASSIGN(Decompilation);
    };
}


#endif //BASE_DECOMPILATION_H
