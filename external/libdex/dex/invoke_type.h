/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_LIBDEXFILE_DEX_INVOKE_TYPE_H_
#define ART_LIBDEXFILE_DEX_INVOKE_TYPE_H_

#include <iosfwd>

namespace libdex {

enum InvokeType : uint32_t {
  kStatic,       // <<static>>
  kDirect,       // <<direct>>
  kVirtual,      // <<virtual>>
  kSuper,        // <<super>>
  kInterface,    // <<interface>>
  kPolymorphic,  // <<polymorphic>>
  kCustom,       // <<custom>>
  kMaxInvokeType = kCustom
};

std::ostream& operator<<(std::ostream& os, const InvokeType& rhs);

}  // namespace libdex

#endif  // ART_LIBDEXFILE_DEX_INVOKE_TYPE_H_
