/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "dex_file_layout.h"

#include "libbase/globals.h"
#include "libbase/bit_utils.h"
#include "libbase/mman.h"
#include "dex_file.h"

namespace libdex {

int DexLayoutSection::MadviseLargestPageAlignedRegion(const uint8_t* begin,
                                                      const uint8_t* end,
                                                      int advice) {
#ifdef _WIN32
  UNUSED(begin);
  UNUSED(end);
  UNUSED(advice);
  PLOG(WARNING) << "madvise is unsupported on Windows.";
#else
  DCHECK_LE(begin, end);
  begin = base::AlignUp(begin, base::kPageSize);
  end = base::AlignDown(end, base::kPageSize);
  if (begin < end) {
    // TODO: remove the direct dependency on madvise here.
    int result = madvise(const_cast<uint8_t*>(begin), end - begin, advice);
    if (result != 0) {
      PLOG(WARNING) << "madvise failed " << result;
    }
    return result;
  }
#endif
  return 0;
}

void DexLayoutSection::Subsection::Madvise(const DexFile* dex_file, int advice) const {
  DCHECK(dex_file != nullptr);
  DCHECK_LT(start_offset_, dex_file->Size());
  DCHECK_LE(end_offset_, dex_file->Size());
  MadviseLargestPageAlignedRegion(dex_file->Begin() + start_offset_,
                                  dex_file->Begin() + end_offset_,
                                  advice);
}

void DexLayoutSections::Madvise(const DexFile* dex_file, MadviseState state) const {
#ifdef _WIN32
  UNUSED(dex_file);
  UNUSED(state);
  PLOG(WARNING) << "madvise is unsupported on Windows.";
#else
  // The dex file is already defaulted to random access everywhere.
  for (const DexLayoutSection& section : sections_) {
    switch (state) {
      case MadviseState::kMadviseStateAtLoad: {
        section.parts_[static_cast<size_t>(LayoutType::kLayoutTypeStartupOnly)].Madvise(
            dex_file,
            MADV_WILLNEED);
        section.parts_[static_cast<size_t>(LayoutType::kLayoutTypeHot)].Madvise(
            dex_file,
            MADV_WILLNEED);
        break;
      }
      case MadviseState::kMadviseStateFinishedLaunch: {
        section.parts_[static_cast<size_t>(LayoutType::kLayoutTypeStartupOnly)].Madvise(
            dex_file,
            MADV_DONTNEED);
        break;
      }
      case MadviseState::kMadviseStateFinishedTrim: {
        section.parts_[static_cast<size_t>(LayoutType::kLayoutTypeSometimesUsed)].Madvise(
            dex_file,
            MADV_DONTNEED);
        section.parts_[static_cast<size_t>(LayoutType::kLayoutTypeUsedOnce)].Madvise(
            dex_file,
            MADV_DONTNEED);
        break;
      }
    }
  }
#endif
}

std::ostream& operator<<(std::ostream& os, const DexLayoutSection& section) {
  for (size_t i = 0; i < static_cast<size_t>(LayoutType::kLayoutTypeCount); ++i) {
    const DexLayoutSection::Subsection& part = section.parts_[i];

  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const DexLayoutSections& sections) {
  for (size_t i = 0; i < static_cast<size_t>(DexLayoutSections::SectionType::kSectionCount); ++i) {
  }
  return os;
}

}  // namespace libdex
