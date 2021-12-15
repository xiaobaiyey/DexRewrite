//
// Created by xiaobai on 2021/11/2.
//

#ifndef MXSHELL_TOOLS_DEX_VERIFY_H
#define MXSHELL_TOOLS_DEX_VERIFY_H

#include "header.h"
namespace dex_ir{
    // Check that the output dex file contains the same data as the original.
// Compares the dex IR of both dex files. Allows the dex files to have different layouts.
    bool VerifyOutputDexFile(dex_ir::Header* orig_header,
                             dex_ir::Header* output_header,
                             std::string* error_msg);

    template<class T> bool VerifyIds(dex_ir::CollectionVector<T>& orig,
                                     dex_ir::CollectionVector<T>& output,
                                     const char* section_name,
                                     std::string* error_msg);
    bool VerifyId(dex_ir::StringId* orig, dex_ir::StringId* output, std::string* error_msg);
    bool VerifyId(dex_ir::TypeId* orig, dex_ir::TypeId* output, std::string* error_msg);
    bool VerifyId(dex_ir::ProtoId* orig, dex_ir::ProtoId* output, std::string* error_msg);
    bool VerifyId(dex_ir::FieldId* orig, dex_ir::FieldId* output, std::string* error_msg);
    bool VerifyId(dex_ir::MethodId* orig, dex_ir::MethodId* output, std::string* error_msg);

    bool VerifyClassDefs(dex_ir::CollectionVector<dex_ir::ClassDef>& orig,
                         dex_ir::CollectionVector<dex_ir::ClassDef>& output,
                         std::string* error_msg);
    bool VerifyClassDef(dex_ir::ClassDef* orig, dex_ir::ClassDef* output, std::string* error_msg);

    bool VerifyTypeList(const dex_ir::TypeList* orig, const dex_ir::TypeList* output);

    bool VerifyAnnotationsDirectory(dex_ir::AnnotationsDirectoryItem* orig,
                                    dex_ir::AnnotationsDirectoryItem* output,
                                    std::string* error_msg);
    bool VerifyFieldAnnotations(dex_ir::FieldAnnotationVector* orig,
                                dex_ir::FieldAnnotationVector* output,
                                uint32_t orig_offset,
                                std::string* error_msg);
    bool VerifyMethodAnnotations(dex_ir::MethodAnnotationVector* orig,
                                 dex_ir::MethodAnnotationVector* output,
                                 uint32_t orig_offset,
                                 std::string* error_msg);
    bool VerifyParameterAnnotations(dex_ir::ParameterAnnotationVector* orig,
                                    dex_ir::ParameterAnnotationVector* output,
                                    uint32_t orig_offset,
                                    std::string* error_msg);
    bool VerifyAnnotationSetRefList(dex_ir::AnnotationSetRefList* orig,
                                    dex_ir::AnnotationSetRefList* output,
                                    std::string* error_msg);
    bool VerifyAnnotationSet(dex_ir::AnnotationSetItem* orig,
                             dex_ir::AnnotationSetItem* output,
                             std::string* error_msg);
    bool VerifyAnnotation(dex_ir::AnnotationItem* orig,
                          dex_ir::AnnotationItem* output,
                          std::string* error_msg);
    bool VerifyEncodedAnnotation(dex_ir::EncodedAnnotation* orig,
                                 dex_ir::EncodedAnnotation* output,
                                 uint32_t orig_offset,
                                 std::string* error_msg);
    bool VerifyAnnotationElement(dex_ir::AnnotationElement* orig,
                                 dex_ir::AnnotationElement* output,
                                 uint32_t orig_offset,
                                 std::string* error_msg);
    bool VerifyEncodedValue(dex_ir::EncodedValue* orig,
                            dex_ir::EncodedValue* output,
                            uint32_t orig_offset,
                            std::string* error_msg);
    bool VerifyEncodedArray(dex_ir::EncodedArrayItem* orig,
                            dex_ir::EncodedArrayItem* output,
                            std::string* error_msg);

    bool VerifyClassData(dex_ir::ClassData* orig, dex_ir::ClassData* output, std::string* error_msg);
    bool VerifyFields(dex_ir::FieldItemVector* orig,
                      dex_ir::FieldItemVector* output,
                      uint32_t orig_offset,
                      std::string* error_msg);
    bool VerifyMethods(dex_ir::MethodItemVector* orig,
                       dex_ir::MethodItemVector* output,
                       uint32_t orig_offset,
                       std::string* error_msg);
    bool VerifyCode(dex_ir::CodeItem* orig, dex_ir::CodeItem* output, std::string* error_msg);
    bool VerifyDebugInfo(dex_ir::DebugInfoItem* orig,
                         dex_ir::DebugInfoItem* output,
                         std::string* error_msg);
    bool VerifyTries(dex_ir::TryItemVector* orig,
                     dex_ir::TryItemVector* output,
                     uint32_t orig_offset,
                     std::string* error_msg);
    bool VerifyHandlers(dex_ir::CatchHandlerVector* orig,
                        dex_ir::CatchHandlerVector* output,
                        uint32_t orig_offset,
                        std::string* error_msg);
    bool VerifyHandler(const dex_ir::CatchHandler* orig,
                       const dex_ir::CatchHandler* output,
                       uint32_t orig_offset,
                       std::string* error_msg);
}


#endif //MXSHELL_TOOLS_DEX_VERIFY_H
