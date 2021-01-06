/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hidden_api_finder.h"

#include "dex/class_accessor-inl.h"
#include "dex/code_item_accessors-inl.h"
#include "dex/dex_instruction-inl.h"
#include "dex/dex_file.h"
#include "dex/method_reference.h"
#include "hidden_api.h"
#include "resolver.h"
#include "veridex.h"

#include <iostream>

namespace dex_ir {

    void HiddenApiFinder::CheckMethod(uint32_t method_id,
                                      VeridexResolver *resolver,
                                      libdex::MethodReference ref) {
        // Note: we always query whether a method is in a list, as the app
        // might define blacklisted APIs (which won't be used at runtime).
        std::string name = HiddenApi::GetApiMethodName(resolver->GetDexFile(), method_id);
        if (hidden_api_.IsInAnyList(name)) {
            method_locations_[name].push_back(ref);
        }
    }

    void HiddenApiFinder::CheckField(uint32_t field_id,
                                     VeridexResolver *resolver,
                                     libdex::MethodReference ref) {
        // Note: we always query whether a field is in a list, as the app
        // might define blacklisted APIs (which won't be used at runtime).
        std::string name = HiddenApi::GetApiFieldName(resolver->GetDexFile(), field_id);
        if (hidden_api_.IsInAnyList(name)) {
            field_locations_[name].push_back(ref);
        }
    }

    void HiddenApiFinder::CollectAccesses(VeridexResolver *resolver) {
        const libdex::DexFile &dex_file = resolver->GetDexFile();
        // Look at all types referenced in this dex file. Any of these
        // types can lead to being used through reflection.
        for (uint32_t i = 0; i < dex_file.NumTypeIds(); ++i) {
            std::string name(dex_file.StringByTypeIdx(libdex::dex::TypeIndex(i)));
            if (hidden_api_.IsInAnyList(name)) {
                classes_.insert(name);
            }
        }
        // Note: we collect strings constants only referenced in code items as the string table
        // contains other kind of strings (eg types).
        for (libdex::ClassAccessor accessor : dex_file.GetClasses()) {
            for (const libdex::ClassAccessor::Method &method : accessor.GetMethods()) {
                for (const libdex::DexInstructionPcPair &inst : method.GetInstructions()) {
                    switch (inst->Opcode()) {
                        case libdex::Instruction::CONST_STRING: {
                            libdex::dex::StringIndex string_index(inst->VRegB_21c());
                            std::string name = std::string(dex_file.StringDataByIdx(string_index));
                            // Cheap filtering on the string literal. We know it cannot be a field/method/class
                            // if it contains a space.
                            if (name.find(' ') == std::string::npos) {
                                // Class names at the Java level are of the form x.y.z, but the list encodes
                                // them of the form Lx/y/z;. Inner classes have '$' for both Java level class
                                // names in strings, and hidden API lists.
                                std::string str = HiddenApi::ToInternalName(name);
                                // Note: we can query the lists directly, as HiddenApi added classes that own
                                // private methods and fields in them.
                                // We don't add class names to the `strings_` set as we know method/field names
                                // don't have '.' or '/'. All hidden API class names have a '/'.
                                if (hidden_api_.IsInAnyList(str)) {
                                    classes_.insert(str);
                                } else if (hidden_api_.IsInAnyList(name)) {
                                    // Could be something passed to JNI.
                                    classes_.insert(name);
                                } else {
                                    // We only keep track of the location for strings, as these will be the
                                    // field/method names the user is interested in.
                                    strings_.insert(name);
                                    reflection_locations_[name].push_back(method.GetReference());
                                }
                            }
                            break;
                        }
                        case libdex::Instruction::INVOKE_DIRECT:
                        case libdex::Instruction::INVOKE_INTERFACE:
                        case libdex::Instruction::INVOKE_STATIC:
                        case libdex::Instruction::INVOKE_SUPER:
                        case libdex::Instruction::INVOKE_VIRTUAL: {
                            CheckMethod(inst->VRegB_35c(), resolver, method.GetReference());
                            break;
                        }

                        case libdex::Instruction::INVOKE_DIRECT_RANGE:
                        case libdex::Instruction::INVOKE_INTERFACE_RANGE:
                        case libdex::Instruction::INVOKE_STATIC_RANGE:
                        case libdex::Instruction::INVOKE_SUPER_RANGE:
                        case libdex::Instruction::INVOKE_VIRTUAL_RANGE: {
                            CheckMethod(inst->VRegB_3rc(), resolver, method.GetReference());
                            break;
                        }

                        case libdex::Instruction::IGET:
                        case libdex::Instruction::IGET_WIDE:
                        case libdex::Instruction::IGET_OBJECT:
                        case libdex::Instruction::IGET_BOOLEAN:
                        case libdex::Instruction::IGET_BYTE:
                        case libdex::Instruction::IGET_CHAR:
                        case libdex::Instruction::IGET_SHORT: {
                            CheckField(inst->VRegC_22c(), resolver, method.GetReference());
                            break;
                        }

                        case libdex::Instruction::IPUT:
                        case libdex::Instruction::IPUT_WIDE:
                        case libdex::Instruction::IPUT_OBJECT:
                        case libdex::Instruction::IPUT_BOOLEAN:
                        case libdex::Instruction::IPUT_BYTE:
                        case libdex::Instruction::IPUT_CHAR:
                        case libdex::Instruction::IPUT_SHORT: {
                            CheckField(inst->VRegC_22c(), resolver, method.GetReference());
                            break;
                        }

                        case libdex::Instruction::SGET:
                        case libdex::Instruction::SGET_WIDE:
                        case libdex::Instruction::SGET_OBJECT:
                        case libdex::Instruction::SGET_BOOLEAN:
                        case libdex::Instruction::SGET_BYTE:
                        case libdex::Instruction::SGET_CHAR:
                        case libdex::Instruction::SGET_SHORT: {
                            CheckField(inst->VRegB_21c(), resolver, method.GetReference());
                            break;
                        }

                        case libdex::Instruction::SPUT:
                        case libdex::Instruction::SPUT_WIDE:
                        case libdex::Instruction::SPUT_OBJECT:
                        case libdex::Instruction::SPUT_BOOLEAN:
                        case libdex::Instruction::SPUT_BYTE:
                        case libdex::Instruction::SPUT_CHAR:
                        case libdex::Instruction::SPUT_SHORT: {
                            CheckField(inst->VRegB_21c(), resolver, method.GetReference());
                            break;
                        }

                        default:
                            break;
                    }
                }
            }
        }
    }

    void HiddenApiFinder::Run(const std::vector<std::unique_ptr<VeridexResolver>> &resolvers) {
        for (const std::unique_ptr<VeridexResolver> &resolver : resolvers) {
            CollectAccesses(resolver.get());
        }
    }

    void HiddenApiFinder::Dump(std::ostream &os,
                               HiddenApiStats *stats,
                               bool dump_reflection, std::map<std::string, std::vector<std::string>> &greylist,
                               std::map<std::string, std::vector<std::string>> &blacklist,
                               std::map<std::string, std::vector<std::string>> &greylist_max_o,
                               std::map<std::string, std::vector<std::string>> &greylist_max_p) {
        stats->linking_count = method_locations_.size() + field_locations_.size();

        // Dump methods from hidden APIs linked against.
        for (const std::pair<const std::string,
                    std::vector<libdex::MethodReference>> &pair : method_locations_) {
            libdex::hiddenapi::ApiList api_list = hidden_api_.GetApiList(pair.first);
            CHECK(api_list.IsValid());
            stats->api_counts[api_list.GetIntValue()]++;
           /* os << "#" << ++stats->count << ": Linking method" << api_list << " " << pair.first << " use(s):";
            os << std::endl;*/
            switch (api_list.GetValue()) {
                case libdex::hiddenapi::ApiList::Value::kGreylist: {
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    greylist[pair.first] = list;
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kBlacklist: {
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    blacklist[pair.first] = list;
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kGreylistMaxO: {
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    greylist_max_o[pair.first] = list;
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kGreylistMaxP: {
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    greylist_max_p[pair.first] = list;
                }
                    break;
                default:
                    break;
            }

            /* HiddenApiFinder::DumpReferences(os, pair.second);
             os << std::endl;*/
        }

        // Dump fields from hidden APIs linked against.
        for (const std::pair<const std::string,
                    std::vector<libdex::MethodReference>> &pair : field_locations_) {
            libdex::hiddenapi::ApiList api_list = hidden_api_.GetApiList(pair.first);
            CHECK(api_list.IsValid());
            stats->api_counts[api_list.GetIntValue()]++;
            /*os << "#" << ++stats->count << ": Linking field" << api_list << " " << pair.first << " use(s):";
            os << std::endl;*/
            switch (api_list.GetValue()) {
                case libdex::hiddenapi::ApiList::Value::kGreylist: {
                    auto name = pair.first;
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    greylist.insert(std::make_pair(name, list));
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kBlacklist: {
                    auto name = pair.first;
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    blacklist.insert(std::make_pair(name, list));
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kGreylistMaxO: {
                    auto name = pair.first;
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    greylist_max_o.insert(std::make_pair(name, list));
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kGreylistMaxP: {
                    auto name = pair.first;
                    std::vector<std::string> list;
                    HiddenApiFinder::DumpReferences(os, pair.second, list);
                    greylist_max_p.insert(std::make_pair(name, list));
                }
                    break;
                default:
                    break;
            }

        }

        if (dump_reflection) {
            // Dump potential reflection uses.
            for (const std::string &cls : classes_) {
                for (const std::string &name : strings_) {
                    std::string full_name = cls + "->" + name;
                    libdex::hiddenapi::ApiList api_list = hidden_api_.GetApiList(full_name);
                    if (api_list.IsValid()) {
                        stats->api_counts[api_list.GetIntValue()]++;
                        stats->reflection_count++;
                       /* os << "#" << ++stats->count << ": Reflection " << api_list << " " << full_name
                           << " potential use(s):";
                        os << std::endl;*/
                        switch (api_list.GetValue()) {
                            case libdex::hiddenapi::ApiList::Value::kGreylist: {
                                std::vector<std::string> list;
                                HiddenApiFinder::DumpReferences(os, reflection_locations_[name], list);
                                greylist.insert(std::make_pair(full_name, list));
                            }
                                break;
                            case libdex::hiddenapi::ApiList::Value::kBlacklist: {
                                std::vector<std::string> list;
                                HiddenApiFinder::DumpReferences(os, reflection_locations_[name], list);
                                blacklist.insert(std::make_pair(full_name, list));
                            }
                                break;
                            case libdex::hiddenapi::ApiList::Value::kGreylistMaxO: {
                                std::vector<std::string> list;
                                HiddenApiFinder::DumpReferences(os, reflection_locations_[name], list);
                                greylist_max_o.insert(std::make_pair(full_name, list));
                            }
                                break;
                            case libdex::hiddenapi::ApiList::Value::kGreylistMaxP: {
                                std::vector<std::string> list;
                                HiddenApiFinder::DumpReferences(os, reflection_locations_[name], list);
                                greylist_max_p.insert(std::make_pair(full_name, list));
                            }
                                break;
                            default:
                                break;
                        }


                    }
                }
            }
        }
    }

    void HiddenApiFinder::DumpReferences(std::ostream &os,
                                         const std::vector<libdex::MethodReference> &references,
                                         std::vector<std::string> &list) {
        static const char *kPrefix = "       ";

        // Count number of occurrences of each reference, to make the output clearer.
        std::map<std::string, size_t> counts;
        for (const libdex::MethodReference &ref : references) {
            std::string ref_string = HiddenApi::GetApiMethodName(ref);
            if (!counts.count(ref_string)) {
                counts[ref_string] = 0;
            }
            counts[ref_string]++;
        }

        for (const std::pair<const std::string, size_t> &pair : counts) {

            list.push_back(pair.first);
      /*      os << kPrefix << pair.first;
            if (pair.second > 1) {
                os << " (" << pair.second << " occurrences)";
            }
            os << std::endl;*/
        }
    }

}  // namespace dex_ir
