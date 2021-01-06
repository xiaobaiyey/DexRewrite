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

#ifndef ART_TOOLS_VERIDEX_HIDDEN_API_FINDER_H_
#define ART_TOOLS_VERIDEX_HIDDEN_API_FINDER_H_

#include "dex/method_reference.h"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include "dex/hiddenapi_flags.h"

namespace dex_ir {

class HiddenApi;
struct HiddenApiStats;
class VeridexResolver;

/**
 * Reports potential uses of hidden APIs from static linking and reflection.
 */
class HiddenApiFinder {
 public:
  explicit HiddenApiFinder(const HiddenApi& hidden_api) : hidden_api_(hidden_api) {}

  // Iterate over the dex files associated with the passed resolvers to report
  // hidden API uses.
  void Run(const std::vector<std::unique_ptr<VeridexResolver>>& app_resolvers);

  void Dump(std::ostream& os, HiddenApiStats* stats, bool dump_reflection,std::map<std::string, std::vector<std::string>> &greylist,
            std::map<std::string, std::vector<std::string>> &blacklist,
            std::map<std::string, std::vector<std::string>> &greylist_max_o,
            std::map<std::string, std::vector<std::string>> &greylist_max_p);

 private:
  void CollectAccesses(VeridexResolver* resolver);
  void CheckMethod(uint32_t method_idx, VeridexResolver* resolver, libdex::MethodReference ref);
  void CheckField(uint32_t field_idx, VeridexResolver* resolver, libdex::MethodReference ref);
  void DumpReferences(std::ostream& os, const std::vector<libdex::MethodReference>& references,std::vector<std::string> &list);

  const HiddenApi& hidden_api_;
  std::set<std::string> classes_;
  std::set<std::string> strings_;
  std::map<std::string, std::vector<libdex::MethodReference>> reflection_locations_;
  std::map<std::string, std::vector<libdex::MethodReference>> method_locations_;
  std::map<std::string, std::vector<libdex::MethodReference>> field_locations_;
};

}  // namespace dex_ir

#endif  // ART_TOOLS_VERIDEX_HIDDEN_API_FINDER_H_
