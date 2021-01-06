//
// Created by xiaob on 2020/1/2.
//

#ifndef BASE_DEXLAYOUT_H
#define BASE_DEXLAYOUT_H

#include <stdint.h>
#include <vector>
#include "dex_container.h"
#include "dex/dex_file.h"
#include "annotation.h"
#include "code_item.h"
#include "proto_id.h"
#include "dex/dex_instruction.h"
#include "dex/dex_file_layout.h"

namespace dex_ir {
    class Header;

    class DexLayout {
    public:
        class VectorOutputContainer {
        public:
            // Begin is not necessarily aligned (for now).
            uint8_t *Begin() {
                return &data_[0];
            }

        private:
            std::vector<uint8_t> data_;
        };


        // Setting this to false disables class def layout entirely, which is stronger than strictly
        // necessary to ensure the partial order w.r.t. class derivation. TODO: Re-enable (b/68317550).
        static constexpr bool kChangeClassDefOrder = false;

        DexLayout(FILE *out_file,
                  dex_ir::Header *header)
                :
                out_file_(out_file),
                header_(header) {}

        int ProcessFile(const char *file_name);

        bool ProcessDexFile(const char *file_name,
                            const libdex::DexFile *dex_file,
                            size_t dex_file_index,
                            std::unique_ptr<DexContainer> *dex_container,
                            std::string *error_msg);

        dex_ir::Header *GetHeader() const { return header_; }

        void SetHeader(dex_ir::Header *header) { header_ = header; }

        libdex::DexLayoutSections &GetSections() {
            return dex_sections_;
        }


    private:
        void DumpAnnotationSetItem(dex_ir::AnnotationSetItem *set_item);

        void DumpBytecodes(uint32_t idx, const dex_ir::CodeItem *code, uint32_t code_offset);

        void DumpCatches(const dex_ir::CodeItem *code);

        void DumpClass(int idx, char **last_package);

        void DumpClassAnnotations(int idx);

        void DumpClassDef(int idx);

        void DumpCode(uint32_t idx,
                      const dex_ir::CodeItem *code,
                      uint32_t code_offset,
                      const char *declaring_class_descriptor,
                      const char *method_name,
                      bool is_static,
                      const dex_ir::ProtoId *proto);

        void DumpEncodedAnnotation(dex_ir::EncodedAnnotation *annotation);

        void DumpEncodedValue(const dex_ir::EncodedValue *data);

        void DumpFileHeader();

        void DumpIField(uint32_t idx, uint32_t flags, uint32_t hiddenapi_flags, int i);

        void DumpInstruction(const dex_ir::CodeItem *code,
                             uint32_t code_offset,
                             uint32_t insn_idx,
                             uint32_t insn_width,
                             const libdex::Instruction *dec_insn);

        void DumpInterface(const dex_ir::TypeId *type_item, int i);

        void DumpLocalInfo(const dex_ir::CodeItem *code);

        void DumpMethod(uint32_t idx,
                        uint32_t flags,
                        uint32_t hiddenapi_flags,
                        const dex_ir::CodeItem *code,
                        int i);

        void DumpPositionInfo(const dex_ir::CodeItem *code);

        void DumpSField(uint32_t idx,
                        uint32_t flags,
                        uint32_t hiddenapi_flags,
                        int i,
                        dex_ir::EncodedValue *init);

        void DumpDexFile();

        void LayoutClassDefsAndClassData(const libdex::DexFile *dex_file);

        void LayoutCodeItems(const libdex::DexFile *dex_file);

        void LayoutStringData(const libdex::DexFile *dex_file);

        // Creates a new layout for the dex file based on profile info.
        // Currently reorders ClassDefs, ClassDataItems, and CodeItems.
        void LayoutOutputFile(const libdex::DexFile *dex_file);

        bool OutputDexFile(const libdex::DexFile *input_dex_file,
                           bool compute_offsets,
                           std::unique_ptr<DexContainer> *dex_container,
                           std::string *error_msg);

        void DumpCFG(const libdex::DexFile *dex_file, int idx);

        void DumpCFG(const libdex::DexFile *dex_file, uint32_t dex_method_idx, const libdex::dex::CodeItem *code);

        FILE *out_file_;
        dex_ir::Header *header_;
        libdex::DexLayoutSections dex_sections_;
        DISALLOW_COPY_AND_ASSIGN(DexLayout);
    };
}


#endif //BASE_DEXLAYOUT_H
