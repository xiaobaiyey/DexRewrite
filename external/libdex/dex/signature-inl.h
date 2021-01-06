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

#ifndef ART_LIBDEXFILE_DEX_SIGNATURE_INL_H_
#define ART_LIBDEXFILE_DEX_SIGNATURE_INL_H_

#include "signature.h"

#include "dex_file-inl.h"

namespace libdex {

inline bool Signature::operator==(const Signature& rhs) const {
  if (dex_file_ == nullptr) {
    return rhs.dex_file_ == nullptr;
  }
  if (rhs.dex_file_ == nullptr) {
    return false;
  }
  if (dex_file_ == rhs.dex_file_) {
    return proto_id_ == rhs.proto_id_;
  }
  uint32_t lhs_shorty_len;  // For a shorty utf16 length == mutf8 length.
  const char* lhs_shorty_data = dex_file_->StringDataAndUtf16LengthByIdx(proto_id_->shorty_idx_,
                                                                         &lhs_shorty_len);
  std::string_view lhs_shorty(lhs_shorty_data, lhs_shorty_len);
  {
    uint32_t rhs_shorty_len;
    const char* rhs_shorty_data =
        rhs.dex_file_->StringDataAndUtf16LengthByIdx(rhs.proto_id_->shorty_idx_,
                                                     &rhs_shorty_len);
    std::string_view rhs_shorty(rhs_shorty_data, rhs_shorty_len);
    if (lhs_shorty != rhs_shorty) {
      return false;  // Shorty mismatch.
    }
  }
  if (lhs_shorty[0] == 'L') {
    const dex::TypeId& return_type_id = dex_file_->GetTypeId(proto_id_->return_type_idx_);
    const dex::TypeId& rhs_return_type_id =
        rhs.dex_file_->GetTypeId(rhs.proto_id_->return_type_idx_);
    if (!DexFile::StringEquals(dex_file_, return_type_id.descriptor_idx_,
                               rhs.dex_file_, rhs_return_type_id.descriptor_idx_)) {
      return false;  // Return type mismatch.
    }
  }
  if (lhs_shorty.find('L', 1) != std::string_view::npos) {
    const dex::TypeList* params = dex_file_->GetProtoParameters(*proto_id_);
    const dex::TypeList* rhs_params = rhs.dex_file_->GetProtoParameters(*rhs.proto_id_);
    // We found a reference parameter in the matching shorty, so both lists must be non-empty.
    DCHECK(params != nullptr);
    DCHECK(rhs_params != nullptr);
    uint32_t params_size = params->Size();
    DCHECK_EQ(params_size, rhs_params->Size());  // Parameter list size must match.
    for (uint32_t i = 0; i < params_size; ++i) {
      const dex::TypeId& param_id = dex_file_->GetTypeId(params->GetTypeItem(i).type_idx_);
      const dex::TypeId& rhs_param_id =
          rhs.dex_file_->GetTypeId(rhs_params->GetTypeItem(i).type_idx_);
      if (!DexFile::StringEquals(dex_file_, param_id.descriptor_idx_,
                                 rhs.dex_file_, rhs_param_id.descriptor_idx_)) {
        return false;  // Parameter type mismatch.
      }
    }
  }
  return true;
}

}  // namespace libdex

#endif  // ART_LIBDEXFILE_DEX_SIGNATURE_INL_H_
