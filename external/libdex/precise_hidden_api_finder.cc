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

#include "precise_hidden_api_finder.h"

#include "dex/class_accessor-inl.h"
#include "dex/code_item_accessors-inl.h"
#include "dex/dex_instruction-inl.h"
#include "dex/dex_file.h"
#include "dex/method_reference.h"
#include "flow_analysis.h"
#include "hidden_api.h"
#include "resolver.h"
#include "veridex.h"

#include <iostream>

namespace dex_ir {

    void PreciseHiddenApiFinder::RunInternal(
            const std::vector<std::unique_ptr<VeridexResolver>> &resolvers,
            const std::function<void(VeridexResolver *, const libdex::ClassAccessor::Method &)> &action) {
        for (const std::unique_ptr<VeridexResolver> &resolver : resolvers) {
            for (libdex::ClassAccessor accessor : resolver->GetDexFile().GetClasses()) {
                for (const libdex::ClassAccessor::Method &method : accessor.GetMethods()) {
                    if (method.GetCodeItem() != nullptr) {
                        action(resolver.get(), method);
                    }
                }
            }
        }
    }

    void PreciseHiddenApiFinder::AddUsesAt(const std::vector<ReflectAccessInfo> &accesses,
                                           libdex::MethodReference ref) {
        for (const ReflectAccessInfo &info : accesses) {
            if (info.IsConcrete()) {
                concrete_uses_[ref].push_back(info);
            } else {
                abstract_uses_[ref].push_back(info);
            }
        }
    }

    void PreciseHiddenApiFinder::Run(const std::vector<std::unique_ptr<VeridexResolver>> &resolvers) {
        // Collect reflection uses.
        RunInternal(resolvers, [this](VeridexResolver *resolver, const libdex::ClassAccessor::Method &method) {
            FlowAnalysisCollector collector(resolver, method);
            collector.Run();
            AddUsesAt(collector.GetUses(), method.GetReference());
        });

        // For non-final reflection uses, do a limited fixed point calculation over the code to try
        // substituting them with final reflection uses.
        // We limit the number of times we iterate over the code as one run can be long.
        static const int kMaximumIterations = 10;
        uint32_t i = 0;
        while (!abstract_uses_.empty() && (i++ < kMaximumIterations)) {
            // Fetch and clear the worklist.
            std::map<libdex::MethodReference, std::vector<ReflectAccessInfo>> current_uses
                    = std::move(abstract_uses_);
            RunInternal(resolvers,
                        [this, current_uses](VeridexResolver *resolver,
                                             const libdex::ClassAccessor::Method &method) {
                            FlowAnalysisSubstitutor substitutor(resolver, method, current_uses);
                            substitutor.Run();
                            AddUsesAt(substitutor.GetUses(), method.GetReference());
                        });
        }
    }

    void PreciseHiddenApiFinder::Dump(std::ostream &os, HiddenApiStats *stats,
                                      std::map<std::string, std::vector<std::string>> &greylist,
                                      std::map<std::string, std::vector<std::string>> &blacklist,
                                      std::map<std::string, std::vector<std::string>> &greylist_max_o,
                                      std::map<std::string, std::vector<std::string>> &greylist_max_p) {
        static const char *kPrefix = "       ";
        std::map<std::string, std::vector<libdex::MethodReference>> named_uses;
        for (auto &it : concrete_uses_) {
            libdex::MethodReference ref = it.first;
            for (const ReflectAccessInfo &info : it.second) {
                std::string cls(info.cls.ToString());
                std::string name(info.name.ToString());
                std::string full_name = cls + "->" + name;
                if (hidden_api_.IsInAnyList(full_name)) {
                    named_uses[full_name].push_back(ref);
                }
            }
        }

        for (auto &it : named_uses) {
            ++stats->reflection_count;
            const std::string &full_name = it.first;
            libdex::hiddenapi::ApiList api_list = hidden_api_.GetApiList(full_name);
            stats->api_counts[api_list.GetIntValue()]++;
           /* os << "#" << ++stats->count << ": Reflection " << api_list << " " << full_name << " use(s):";
            os << std::endl;*/
            std::vector<std::string> list;
            for (const libdex::MethodReference &ref : it.second) {
                list.push_back(HiddenApi::GetApiMethodName(ref));
                //os << kPrefix << HiddenApi::GetApiMethodName(ref) << std::endl;
            }
            switch (api_list.GetValue()) {
                case libdex::hiddenapi::ApiList::Value::kGreylist: {
                    greylist.insert(std::make_pair(full_name, list));
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kBlacklist: {
                    blacklist.insert(std::make_pair(full_name, list));
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kGreylistMaxO: {
                    greylist_max_o.insert(std::make_pair(full_name, list));
                }
                    break;
                case libdex::hiddenapi::ApiList::Value::kGreylistMaxP: {
                    greylist_max_p.insert(std::make_pair(full_name, list));
                }
                    break;
                default:
                    break;
            }

        }
    }

}  // namespace dex_ir
