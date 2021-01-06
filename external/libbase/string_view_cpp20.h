/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ART_LIBARTBASE_BASE_STRING_VIEW_CPP20_H_
#define ART_LIBARTBASE_BASE_STRING_VIEW_CPP20_H_

#include <string_view>

namespace base {

// Replacement functions for std::string_view::starts_with(), ends_with()
// which shall be available in C++20.
#if __cplusplus >= 202000L
#error "When upgrading to C++20, remove this error and file a bug to remove this workaround."
#endif

inline bool StartsWith(std::string_view sv, std::string_view prefix) {
  return sv.substr(0u, prefix.size()) == prefix;
}

inline bool EndsWith(std::string_view sv, std::string_view suffix) {
  return sv.size() >= suffix.size() && sv.substr(sv.size() - suffix.size()) == suffix;
}

}  // namespace art

#endif  // ART_LIBARTBASE_BASE_STRING_VIEW_CPP20_H_
