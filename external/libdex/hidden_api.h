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

#ifndef ART_TOOLS_VERIDEX_HIDDEN_API_H_
#define ART_TOOLS_VERIDEX_HIDDEN_API_H_


#include "dex/method_reference.h"

#include <map>
#include <ostream>
#include <string>
#include "dex/hiddenapi_flags.h"

namespace dex_ir {

class DexFile;

/**
 * Helper class for logging if a method/field is in a hidden API list.
 */
class HiddenApi {
 public:
  HiddenApi(const char* flags_file, bool sdk_uses_only);

    libdex::hiddenapi::ApiList GetApiList(const std::string& name) const {
    auto it = api_list_.find(name);
    return (it == api_list_.end()) ? libdex::hiddenapi::ApiList() : it->second;
  }

  bool IsInAnyList(const std::string& name) const {
    return !GetApiList(name).IsEmpty();
  }

  static std::string GetApiMethodName(const libdex::DexFile& dex_file, uint32_t method_index);

  static std::string GetApiFieldName(const libdex::DexFile& dex_file, uint32_t field_index);

  static std::string GetApiMethodName(libdex::MethodReference ref) {
    return HiddenApi::GetApiMethodName(*ref.dex_file, ref.index);
  }

  static std::string ToInternalName(const std::string& str) {
    std::string val = str;
    std::replace(val.begin(), val.end(), '.', '/');
    return "L" + val + ";";
  }

 private:
  void AddSignatureToApiList(const std::string& signature, libdex::hiddenapi::ApiList membership);

  std::map<std::string, libdex::hiddenapi::ApiList> api_list_;
};

struct HiddenApiStats {
  uint32_t count = 0;
  uint32_t reflection_count = 0;
  uint32_t linking_count = 0;
  uint32_t api_counts[libdex::hiddenapi::ApiList::kValueCount] = {};  // initialize all to zero
};

}  // namespace dex_ir

#endif  // ART_TOOLS_VERIDEX_HIDDEN_API_H_
