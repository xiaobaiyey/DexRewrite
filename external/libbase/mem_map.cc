/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include "mem_map.h"

#include <inttypes.h>
#include <stdlib.h>
#if !defined(ANDROID_OS) && !defined(__Fuchsia__) && !defined(_WIN32)
#include <sys/resource.h>
#endif

#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include <map>
#include <memory>
#include <sstream>


#include "stringprintf.h"
#include "allocator.h"
#include "bit_utils.h"
#include "globals.h"
#include "logging.h"  // For VLOG_IS_ON.
#include "memory_tool.h"
#include "mman.h"  // For the PROT_* and MAP_* constants.
#include "utils.h"
#include "unique_fd.h"
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

namespace base {

using base::StringPrintf;
using base::unique_fd;

template<class Key, class T, AllocatorTag kTag, class Compare = std::less<Key>>
using AllocationTrackingMultiMap =
    std::multimap<Key, T, Compare, TrackingAllocator<std::pair<const Key, T>, kTag>>;

using Maps = AllocationTrackingMultiMap<void*, MemMap*, kAllocatorTagMaps>;

// All the non-empty MemMaps. Use a multimap as we do a reserve-and-divide (eg ElfMap::Load()).
static Maps* gMaps GUARDED_BY(MemMap::GetMemMapsLock()) = nullptr;

// A map containing unique strings used for indentifying anonymous mappings
static std::map<std::string, int> debugStrMap GUARDED_BY(MemMap::GetMemMapsLock());

// Retrieve iterator to a `gMaps` entry that is known to exist.
Maps::iterator GetGMapsEntry(const MemMap& map) REQUIRES(MemMap::GetMemMapsLock()) {
  DCHECK(map.IsValid());
  DCHECK(gMaps != nullptr);
  for (auto it = gMaps->lower_bound(map.BaseBegin()), end = gMaps->end();
       it != end && it->first == map.BaseBegin();
       ++it) {
    if (it->second == &map) {
      return it;
    }
  }
  LOG(FATAL) << "MemMap not found";
  UNREACHABLE();
}

std::ostream& operator<<(std::ostream& os, const Maps& mem_maps) {
  os << "MemMap:" << std::endl;
  for (auto it = mem_maps.begin(); it != mem_maps.end(); ++it) {
    void* base = it->first;
    MemMap* map = it->second;
    CHECK_EQ(base, map->BaseBegin());
    os << *map << std::endl;
  }
  return os;
}

std::mutex* MemMap::mem_maps_lock_ = nullptr;

#if USE_ART_LOW_4G_ALLOCATOR
// Handling mem_map in 32b address range for 64b architectures that do not support MAP_32BIT.

// The regular start of memory allocations. The first 64KB is protected by SELinux.
static constexpr uintptr_t LOW_MEM_START = 64 * KB;

// Generate random starting position.
// To not interfere with image position, take the image's address and only place it below. Current
// formula (sketch):
//
// ART_BASE_ADDR      = 0001XXXXXXXXXXXXXXX
// ----------------------------------------
//                    = 0000111111111111111
// & ~(kPageSize - 1) =~0000000000000001111
// ----------------------------------------
// mask               = 0000111111111110000
// & random data      = YYYYYYYYYYYYYYYYYYY
// -----------------------------------
// tmp                = 0000YYYYYYYYYYY0000
// + LOW_MEM_START    = 0000000000001000000
// --------------------------------------
// start
//
// arc4random as an entropy source is exposed in Bionic, but not in glibc. When we
// do not have Bionic, simply start with LOW_MEM_START.

// Function is standalone so it can be tested somewhat in mem_map_test.cc.
#ifdef __BIONIC__
uintptr_t CreateStartPos(uint64_t input) {


  // Start with all bits below highest bit in ART_BASE_ADDRESS.
  constexpr size_t leading_zeros = 0;
   uintptr_t mask_ones = (1 << (31 - leading_zeros)) - 1;

  // Lowest (usually 12) bits are not used, as aligned by page size.
   uintptr_t mask = mask_ones & ~(kPageSize - 1);

  // Mask input data.
  return (input & mask) + LOW_MEM_START;
}
#endif

static uintptr_t GenerateNextMemPos() {
#ifdef __BIONIC__
  uint64_t random_data;
  arc4random_buf(&random_data, sizeof(random_data));
  return CreateStartPos(random_data);
#else
  // No arc4random on host, see above.
  return LOW_MEM_START;
#endif
}

// Initialize linear scan to random position.
uintptr_t MemMap::next_mem_pos_ = GenerateNextMemPos();
#endif

// Return true if the address range is contained in a single memory map by either reading
// the gMaps variable or the /proc/self/map entry.
bool MemMap::ContainedWithinExistingMap(uint8_t* ptr, size_t size, std::string* error_msg) {
  uintptr_t begin = reinterpret_cast<uintptr_t>(ptr);
  uintptr_t end = begin + size;

  {
    std::lock_guard<std::mutex> mu(*mem_maps_lock_);
    for (auto& pair : *gMaps) {
      MemMap* const map = pair.second;
      if (begin >= reinterpret_cast<uintptr_t>(map->Begin()) &&
          end <= reinterpret_cast<uintptr_t>(map->End())) {
        return true;
      }
    }
  }

  if (error_msg != nullptr) {

    *error_msg = StringPrintf("Requested region 0x%08" PRIxPTR "-0x%08" PRIxPTR " does not overlap "
                              "any existing map. See process maps in the log.", begin, end);
  }
  return false;
}

// CheckMapRequest to validate a non-MAP_FAILED mmap result based on
// the expected value, calling munmap if validation fails, giving the
// reason in error_msg.
//
// If the expected_ptr is null, nothing is checked beyond the fact
// that the actual_ptr is not MAP_FAILED. However, if expected_ptr is
// non-null, we check that pointer is the actual_ptr == expected_ptr,
// and if not, report in error_msg what the conflict mapping was if
// found, or a generic error in other cases.
bool MemMap::CheckMapRequest(uint8_t* expected_ptr, void* actual_ptr, size_t byte_count,
                            std::string* error_msg) {
  // Handled first by caller for more specific error messages.
  CHECK(actual_ptr != MAP_FAILED);

  if (expected_ptr == nullptr) {
    return true;
  }

  uintptr_t actual = reinterpret_cast<uintptr_t>(actual_ptr);
  uintptr_t expected = reinterpret_cast<uintptr_t>(expected_ptr);

  if (expected_ptr == actual_ptr) {
    return true;
  }

  // We asked for an address but didn't get what we wanted, all paths below here should fail.
  int result = TargetMUnmap(actual_ptr, byte_count);
  if (result == -1) {
    PLOG(WARNING) << StringPrintf("munmap(%p, %zd) failed", actual_ptr, byte_count);
  }

  if (error_msg != nullptr) {
    // We call this here so that we can try and generate a full error
    // message with the overlapping mapping. There's no guarantee that
    // that there will be an overlap though, since
    // - The kernel is not *required* to honor expected_ptr unless MAP_FIXED is
    //   true, even if there is no overlap
    // - There might have been an overlap at the point of mmap, but the
    //   overlapping region has since been unmapped.

    // Tell the client the mappings that were in place at the time.


    std::ostringstream os;
    os <<  StringPrintf("Failed to mmap at expected address, mapped at "
                        "0x%08" PRIxPTR " instead of 0x%08" PRIxPTR,
                        actual, expected);
    *error_msg = os.str();
  }
  return false;
}

bool MemMap::CheckReservation(uint8_t* expected_ptr,
                              size_t byte_count,
                              const char* name,
                              const MemMap& reservation,
                              /*out*/std::string* error_msg) {
  if (!reservation.IsValid()) {
    *error_msg = StringPrintf("Invalid reservation for %s", name);
    return false;
  }
  DCHECK_ALIGNED(reservation.Begin(), kPageSize);
  if (reservation.Begin() != expected_ptr) {
    *error_msg = StringPrintf("Bad image reservation start for %s: %p instead of %p",
                              name,
                              reservation.Begin(),
                              expected_ptr);
    return false;
  }
  if (byte_count > reservation.Size()) {
    *error_msg = StringPrintf("Insufficient reservation, required %zu, available %zu",
                              byte_count,
                              reservation.Size());
    return false;
  }
  return true;
}


#if USE_ART_LOW_4G_ALLOCATOR
void* MemMap::TryMemMapLow4GB(void* ptr,
                                    size_t page_aligned_byte_count,
                                    int prot,
                                    int flags,
                                    int fd,
                                    off_t offset) {
  void* actual = TargetMMap(ptr, page_aligned_byte_count, prot, flags, fd, offset);
  if (actual != MAP_FAILED) {
    // Since we didn't use MAP_FIXED the kernel may have mapped it somewhere not in the low
    // 4GB. If this is the case, unmap and retry.
    if (reinterpret_cast<uintptr_t>(actual) + page_aligned_byte_count >= 4 * GB) {
      TargetMUnmap(actual, page_aligned_byte_count);
      actual = MAP_FAILED;
    }
  }
  return actual;
}
#endif

void MemMap::SetDebugName(void* map_ptr, const char* name, size_t size) {
  // Debug naming is only used for Android target builds. For Linux targets,
  // we'll still call prctl but it wont do anything till we upstream the prctl.


  // lock as std::map is not thread-safe
  std::lock_guard<std::mutex> mu(*mem_maps_lock_);

  std::string debug_friendly_name("dalvik-");
  debug_friendly_name += name;
  auto it = debugStrMap.find(debug_friendly_name);

  if (it == debugStrMap.end()) {
    it = debugStrMap.insert(std::make_pair(std::move(debug_friendly_name), 1)).first;
  }

  DCHECK(it != debugStrMap.end());
#if defined(PR_SET_VMA) && defined(__linux__)
  prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, map_ptr, size, it->first.c_str());
#else
  // Prevent variable unused compiler errors.
  UNUSED(map_ptr, size);
#endif
}

MemMap MemMap::MapAnonymous(const char* name,
                            uint8_t* addr,
                            size_t byte_count,
                            int prot,
                            bool low_4gb,
                            bool reuse,
                            /*inout*/MemMap* reservation,
                            /*out*/std::string* error_msg,
                            bool use_debug_name) {
#ifndef __LP64__
  UNUSED(low_4gb);
#endif
  if (byte_count == 0) {
    *error_msg = "Empty MemMap requested.";
    return Invalid();
  }
  size_t page_aligned_byte_count = RoundUp(byte_count, kPageSize);

  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  if (reuse) {
    // reuse means it is okay that it overlaps an existing page mapping.
    // Only use this if you actually made the page reservation yourself.
    CHECK(addr != nullptr);
    DCHECK(reservation == nullptr);

    DCHECK(ContainedWithinExistingMap(addr, byte_count, error_msg)) << *error_msg;
    flags |= MAP_FIXED;
  } else if (reservation != nullptr) {
    CHECK(addr != nullptr);
    if (!CheckReservation(addr, byte_count, name, *reservation, error_msg)) {
      return MemMap::Invalid();
    }
    flags |= MAP_FIXED;
  }

  unique_fd fd;

  // We need to store and potentially set an error number for pretty printing of errors
  int saved_errno = 0;

  void* actual = MapInternal(addr,
                             page_aligned_byte_count,
                             prot,
                             flags,
                             fd.get(),
                             0,
                             low_4gb);
  saved_errno = errno;

  if (actual == MAP_FAILED) {
    if (error_msg != nullptr) {


      *error_msg = StringPrintf("Failed anonymous mmap(%p, %zd, 0x%x, 0x%x, %d, 0): %s. "
                                    "See process maps in the log.",
                                addr,
                                page_aligned_byte_count,
                                prot,
                                flags,
                                fd.get(),
                                strerror(saved_errno));
    }
    return Invalid();
  }
  if (!CheckMapRequest(addr, actual, page_aligned_byte_count, error_msg)) {
    return Invalid();
  }

  if (use_debug_name) {
    SetDebugName(actual, name, page_aligned_byte_count);
  }

  if (reservation != nullptr) {
    // Re-mapping was successful, transfer the ownership of the memory to the new MemMap.
    DCHECK_EQ(actual, reservation->Begin());
    reservation->ReleaseReservedMemory(byte_count);
  }
  return MemMap(name,
                reinterpret_cast<uint8_t*>(actual),
                byte_count,
                actual,
                page_aligned_byte_count,
                prot,
                reuse);
}

MemMap MemMap::MapDummy(const char* name, uint8_t* addr, size_t byte_count) {
  if (byte_count == 0) {
    return Invalid();
  }
  const size_t page_aligned_byte_count = RoundUp(byte_count, kPageSize);
  return MemMap(name, addr, byte_count, addr, page_aligned_byte_count, 0, /* reuse= */ true);
}

template<typename A, typename B>
static ptrdiff_t PointerDiff(A* a, B* b) {
  return static_cast<ptrdiff_t>(reinterpret_cast<intptr_t>(a) - reinterpret_cast<intptr_t>(b));
}

bool MemMap::ReplaceWith(MemMap* source, /*out*/std::string* error) {
#if !HAVE_MREMAP_SYSCALL
  UNUSED(source);
  *error = "Cannot perform atomic replace because we are missing the required mremap syscall";
  return false;
#else  // !HAVE_MREMAP_SYSCALL
  CHECK(source != nullptr);
  CHECK(source->IsValid());
  if (!MemMap::kCanReplaceMapping) {
    *error = "Unable to perform atomic replace due to runtime environment!";
    return false;
  }
  // neither can be reuse.
  if (source->reuse_ || reuse_) {
    *error = "One or both mappings is not a real mmap!";
    return false;
  }
  // TODO Support redzones.
  if (source->redzone_size_ != 0 || redzone_size_ != 0) {
    *error = "source and dest have different redzone sizes";
    return false;
  }
  // Make sure they have the same offset from the actual mmap'd address
  if (PointerDiff(BaseBegin(), Begin()) != PointerDiff(source->BaseBegin(), source->Begin())) {
    *error =
        "source starts at a different offset from the mmap. Cannot atomically replace mappings";
    return false;
  }
  // mremap doesn't allow the final [start, end] to overlap with the initial [start, end] (it's like
  // memcpy but the check is explicit and actually done).
  if (source->BaseBegin() > BaseBegin() &&
      reinterpret_cast<uint8_t*>(BaseBegin()) + source->BaseSize() >
      reinterpret_cast<uint8_t*>(source->BaseBegin())) {
    *error = "destination memory pages overlap with source memory pages";
    return false;
  }
  // Change the protection to match the new location.
  int old_prot = source->GetProtect();
  if (!source->Protect(GetProtect())) {
    *error = "Could not change protections for source to those required for dest.";
    return false;
  }

  // Do the mremap.
  void* res = mremap(/*old_address*/source->BaseBegin(),
                     /*old_size*/source->BaseSize(),
                     /*new_size*/source->BaseSize(),
                     /*flags*/MREMAP_MAYMOVE | MREMAP_FIXED,
                     /*new_address*/BaseBegin());
  if (res == MAP_FAILED) {
    int saved_errno = errno;
    // Wasn't able to move mapping. Change the protection of source back to the original one and
    // return.
    source->Protect(old_prot);
    *error = std::string("Failed to mremap source to dest. Error was ") + strerror(saved_errno);
    return false;
  }
  CHECK(res == BaseBegin());

  // The new base_size is all the pages of the 'source' plus any remaining dest pages. We will unmap
  // them later.
  size_t new_base_size = std::max(source->base_size_, base_size_);

  // Invalidate *source, don't unmap it though since it is already gone.
  size_t source_size = source->size_;
  source->Invalidate();

  size_ = source_size;
  base_size_ = new_base_size;
  // Reduce base_size if needed (this will unmap the extra pages).
  SetSize(source_size);

  return true;
#endif  // !HAVE_MREMAP_SYSCALL
}

MemMap MemMap::MapFileAtAddress(uint8_t* expected_ptr,
                                size_t byte_count,
                                int prot,
                                int flags,
                                int fd,
                                off_t start,
                                bool low_4gb,
                                const char* filename,
                                bool reuse,
                                /*inout*/MemMap* reservation,
                                /*out*/std::string* error_msg) {
  CHECK_NE(0, prot);
  CHECK_NE(0, flags & (MAP_SHARED | MAP_PRIVATE));

  // Note that we do not allow MAP_FIXED unless reuse == true or we have an existing
  // reservation, i.e we expect this mapping to be contained within an existing map.
  if (reuse) {
    // reuse means it is okay that it overlaps an existing page mapping.
    // Only use this if you actually made the page reservation yourself.
    CHECK(expected_ptr != nullptr);
    DCHECK(reservation == nullptr);
    DCHECK(error_msg != nullptr);
    DCHECK(ContainedWithinExistingMap(expected_ptr, byte_count, error_msg))
        << ((error_msg != nullptr) ? *error_msg : std::string());
    flags |= MAP_FIXED;
  } else if (reservation != nullptr) {
    DCHECK(error_msg != nullptr);
    if (!CheckReservation(expected_ptr, byte_count, filename, *reservation, error_msg)) {
      return Invalid();
    }
    flags |= MAP_FIXED;
  } else {
    CHECK_EQ(0, flags & MAP_FIXED);
    // Don't bother checking for an overlapping region here. We'll
    // check this if required after the fact inside CheckMapRequest.
  }

  if (byte_count == 0) {
    *error_msg = "Empty MemMap requested";
    return Invalid();
  }
  // Adjust 'offset' to be page-aligned as required by mmap.
  int page_offset = start % kPageSize;
  off_t page_aligned_offset = start - page_offset;
  // Adjust 'byte_count' to be page-aligned as we will map this anyway.
  size_t page_aligned_byte_count = RoundUp(byte_count + page_offset, kPageSize);
  // The 'expected_ptr' is modified (if specified, ie non-null) to be page aligned to the file but
  // not necessarily to virtual memory. mmap will page align 'expected' for us.
  uint8_t* page_aligned_expected =
      (expected_ptr == nullptr) ? nullptr : (expected_ptr - page_offset);

  size_t redzone_size = 0;
  if (kRunningOnMemoryTool && kMemoryToolAddsRedzones && expected_ptr == nullptr) {
    redzone_size = kPageSize;
    page_aligned_byte_count += redzone_size;
  }

  uint8_t* actual = reinterpret_cast<uint8_t*>(MapInternal(page_aligned_expected,
                                                           page_aligned_byte_count,
                                                           prot,
                                                           flags,
                                                           fd,
                                                           page_aligned_offset,
                                                           low_4gb));
  if (actual == MAP_FAILED) {
    if (error_msg != nullptr) {
      auto saved_errno = errno;



      *error_msg = StringPrintf("mmap(%p, %zd, 0x%x, 0x%x, %d, %" PRId64
                                ") of file '%s' failed: %s. See process maps in the log.",
                                page_aligned_expected, page_aligned_byte_count, prot, flags, fd,
                                static_cast<int64_t>(page_aligned_offset), filename,
                                strerror(saved_errno));
    }
    return Invalid();
  }
  if (!CheckMapRequest(expected_ptr, actual, page_aligned_byte_count, error_msg)) {
    return Invalid();
  }
  if (redzone_size != 0) {
    const uint8_t *real_start = actual + page_offset;
    const uint8_t *real_end = actual + page_offset + byte_count;
    const uint8_t *mapping_end = actual + page_aligned_byte_count;

    MEMORY_TOOL_MAKE_NOACCESS(actual, real_start - actual);
    MEMORY_TOOL_MAKE_NOACCESS(real_end, mapping_end - real_end);
    page_aligned_byte_count -= redzone_size;
  }

  if (reservation != nullptr) {
    // Re-mapping was successful, transfer the ownership of the memory to the new MemMap.
    DCHECK_EQ(actual, reservation->Begin());
    reservation->ReleaseReservedMemory(byte_count);
  }
  return MemMap(filename,
                actual + page_offset,
                byte_count,
                actual,
                page_aligned_byte_count,
                prot,
                reuse,
                redzone_size);
}

MemMap::MemMap(MemMap&& other) noexcept
    : MemMap() {
  swap(other);
}

MemMap::~MemMap() {
  Reset();
}

void MemMap::DoReset() {
  DCHECK(IsValid());

  // Unlike Valgrind, AddressSanitizer requires that all manually poisoned memory is unpoisoned
  // before it is returned to the system.
  if (redzone_size_ != 0) {
    MEMORY_TOOL_MAKE_UNDEFINED(
        reinterpret_cast<char*>(base_begin_) + base_size_ - redzone_size_,
        redzone_size_);
  }

  if (!reuse_) {
    MEMORY_TOOL_MAKE_UNDEFINED(base_begin_, base_size_);
    if (!already_unmapped_) {
      int result = TargetMUnmap(base_begin_, base_size_);
      if (result == -1) {
        PLOG(FATAL) << "munmap failed";
      }
    }
  }

  Invalidate();
}

void MemMap::Invalidate() {
  DCHECK(IsValid());

  // Remove it from gMaps.
  std::lock_guard<std::mutex> mu(*mem_maps_lock_);
  auto it = GetGMapsEntry(*this);
  gMaps->erase(it);

  // Mark it as invalid.
  base_size_ = 0u;
  DCHECK(!IsValid());
}

void MemMap::swap(MemMap& other) {
  if (IsValid() || other.IsValid()) {
    std::lock_guard<std::mutex> mu(*mem_maps_lock_);
    DCHECK(gMaps != nullptr);
    auto this_it = IsValid() ? GetGMapsEntry(*this) : gMaps->end();
    auto other_it = other.IsValid() ? GetGMapsEntry(other) : gMaps->end();
    if (IsValid()) {
      DCHECK(this_it != gMaps->end());
      DCHECK_EQ(this_it->second, this);
      this_it->second = &other;
    }
    if (other.IsValid()) {
      DCHECK(other_it != gMaps->end());
      DCHECK_EQ(other_it->second, &other);
      other_it->second = this;
    }
    // Swap members with the `mem_maps_lock_` held so that `base_begin_` matches
    // with the `gMaps` key when other threads try to use `gMaps`.
    SwapMembers(other);
  } else {
    SwapMembers(other);
  }
}

void MemMap::SwapMembers(MemMap& other) {
  name_.swap(other.name_);
  std::swap(begin_, other.begin_);
  std::swap(size_, other.size_);
  std::swap(base_begin_, other.base_begin_);
  std::swap(base_size_, other.base_size_);
  std::swap(prot_, other.prot_);
  std::swap(reuse_, other.reuse_);
  std::swap(already_unmapped_, other.already_unmapped_);
  std::swap(redzone_size_, other.redzone_size_);
}

MemMap::MemMap(const std::string& name, uint8_t* begin, size_t size, void* base_begin,
               size_t base_size, int prot, bool reuse, size_t redzone_size)
    : name_(name), begin_(begin), size_(size), base_begin_(base_begin), base_size_(base_size),
      prot_(prot), reuse_(reuse), already_unmapped_(false), redzone_size_(redzone_size) {
  if (size_ == 0) {
    CHECK(begin_ == nullptr);
    CHECK(base_begin_ == nullptr);
    CHECK_EQ(base_size_, 0U);
  } else {
    CHECK(begin_ != nullptr);
    CHECK(base_begin_ != nullptr);
    CHECK_NE(base_size_, 0U);

    // Add it to gMaps.
    std::lock_guard<std::mutex> mu(*mem_maps_lock_);
    DCHECK(gMaps != nullptr);
    gMaps->insert(std::make_pair(base_begin_, this));
  }
}

MemMap MemMap::RemapAtEnd(uint8_t* new_end,
                          const char* tail_name,
                          int tail_prot,
                          std::string* error_msg,
                          bool use_debug_name) {
  return RemapAtEnd(new_end,
                    tail_name,
                    tail_prot,
                    MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
                    /* fd= */ -1,
                    /* offset= */ 0,
                    error_msg,
                    use_debug_name);
}
    void MemMap::TargetMMapInit() {
        // no-op for unix
    }

    void* MemMap::TargetMMap(void* start, size_t len, int prot, int flags, int fd, off_t fd_off) {
        return mmap(start, len, prot, flags, fd, fd_off);
    }

    int MemMap::TargetMUnmap(void* start, size_t len) {
        return munmap(start, len);
    }
MemMap MemMap::RemapAtEnd(uint8_t* new_end,
                          const char* tail_name,
                          int tail_prot,
                          int flags,
                          int fd,
                          off_t offset,
                          std::string* error_msg,
                          bool use_debug_name) {
  DCHECK_GE(new_end, Begin());
  DCHECK_LE(new_end, End());
  DCHECK_LE(begin_ + size_, reinterpret_cast<uint8_t*>(base_begin_) + base_size_);
  DCHECK_ALIGNED(begin_, kPageSize);
  DCHECK_ALIGNED(base_begin_, kPageSize);
  DCHECK_ALIGNED(reinterpret_cast<uint8_t*>(base_begin_) + base_size_, kPageSize);
  DCHECK_ALIGNED(new_end, kPageSize);
  uint8_t* old_end = begin_ + size_;
  uint8_t* old_base_end = reinterpret_cast<uint8_t*>(base_begin_) + base_size_;
  uint8_t* new_base_end = new_end;
  DCHECK_LE(new_base_end, old_base_end);
  if (new_base_end == old_base_end) {
    return Invalid();
  }
  size_t new_size = new_end - reinterpret_cast<uint8_t*>(begin_);
  size_t new_base_size = new_base_end - reinterpret_cast<uint8_t*>(base_begin_);
  DCHECK_LE(begin_ + new_size, reinterpret_cast<uint8_t*>(base_begin_) + new_base_size);
  size_t tail_size = old_end - new_end;
  uint8_t* tail_base_begin = new_base_end;
  size_t tail_base_size = old_base_end - new_base_end;
  DCHECK_EQ(tail_base_begin + tail_base_size, old_base_end);
  DCHECK_ALIGNED(tail_base_size, kPageSize);

  MEMORY_TOOL_MAKE_UNDEFINED(tail_base_begin, tail_base_size);
  // Note: Do not explicitly unmap the tail region, mmap() with MAP_FIXED automatically
  // removes old mappings for the overlapping region. This makes the operation atomic
  // and prevents other threads from racing to allocate memory in the requested region.
  uint8_t* actual = reinterpret_cast<uint8_t*>(TargetMMap(tail_base_begin,
                                                          tail_base_size,
                                                          tail_prot,
                                                          flags,
                                                          fd,
                                                          offset));
  if (actual == MAP_FAILED) {

    *error_msg = StringPrintf("map(%p, %zd, 0x%x, 0x%x, %d, 0) failed. See process "
                              "maps in the log.", tail_base_begin, tail_base_size, tail_prot, flags,
                              fd);
    return Invalid();
  }
  // Update *this.
  if (new_base_size == 0u) {
    std::lock_guard<std::mutex> mu(*mem_maps_lock_);
    auto it = GetGMapsEntry(*this);
    gMaps->erase(it);
  }

  if (use_debug_name) {
    SetDebugName(actual, tail_name, tail_base_size);
  }

  size_ = new_size;
  base_size_ = new_base_size;
  // Return the new mapping.
  return MemMap(tail_name, actual, tail_size, actual, tail_base_size, tail_prot, false);
}

MemMap MemMap::TakeReservedMemory(size_t byte_count) {
  uint8_t* begin = Begin();
  ReleaseReservedMemory(byte_count);  // Performs necessary DCHECK()s on this reservation.
  size_t base_size = RoundUp(byte_count, kPageSize);
  return MemMap(name_, begin, byte_count, begin, base_size, prot_, /* reuse= */ false);
}

void MemMap::ReleaseReservedMemory(size_t byte_count) {
  // Check the reservation mapping.
  DCHECK(IsValid());
  DCHECK(!reuse_);
  DCHECK(!already_unmapped_);
  DCHECK_EQ(redzone_size_, 0u);
  DCHECK_EQ(begin_, base_begin_);
  DCHECK_EQ(size_, base_size_);
  DCHECK_ALIGNED(begin_, kPageSize);
  DCHECK_ALIGNED(size_, kPageSize);

  // Check and round up the `byte_count`.
  DCHECK_NE(byte_count, 0u);
  DCHECK_LE(byte_count, size_);
  byte_count = RoundUp(byte_count, kPageSize);

  if (byte_count == size_) {
    Invalidate();
  } else {
    // Shrink the reservation MemMap and update its `gMaps` entry.
    std::lock_guard<std::mutex> mu(*mem_maps_lock_);
    auto it = GetGMapsEntry(*this);
    auto node = gMaps->extract(it);
    begin_ += byte_count;
    size_ -= byte_count;
    base_begin_ = begin_;
    base_size_ = size_;
    node.key() = base_begin_;
    gMaps->insert(std::move(node));
  }
}

void MemMap::MadviseDontNeedAndZero() {
  if (base_begin_ != nullptr || base_size_ != 0) {
    if (!kMadviseZeroes) {
      memset(base_begin_, 0, base_size_);
    }
#ifdef _WIN32
    // It is benign not to madvise away the pages here.
    PLOG(WARNING) << "MemMap::MadviseDontNeedAndZero does not madvise on Windows.";
#else
    int result = madvise(base_begin_, base_size_, MADV_DONTNEED);
    if (result == -1) {
      PLOG(WARNING) << "madvise failed";
    }
#endif
  }
}

bool MemMap::Sync() {
#ifdef _WIN32
  // TODO: add FlushViewOfFile support.
  PLOG(ERROR) << "MemMap::Sync unsupported on Windows.";
  return false;
#else
  // Historical note: To avoid Valgrind errors, we temporarily lifted the lower-end noaccess
  // protection before passing it to msync() when `redzone_size_` was non-null, as Valgrind
  // only accepts page-aligned base address, and excludes the higher-end noaccess protection
  // from the msync range. b/27552451.
  return msync(BaseBegin(), BaseSize(), MS_SYNC) == 0;
#endif
}

bool MemMap::Protect(int prot) {
  if (base_begin_ == nullptr && base_size_ == 0) {
    prot_ = prot;
    return true;
  }

#ifndef _WIN32
  if (mprotect(base_begin_, base_size_, prot) == 0) {
    prot_ = prot;
    return true;
  }
#endif

  PLOG(ERROR) << "mprotect(" << reinterpret_cast<void*>(base_begin_) << ", " << base_size_ << ", "
              << prot << ") failed";
  return false;
}

bool MemMap::CheckNoGaps(MemMap& begin_map, MemMap& end_map) {
  std::lock_guard<std::mutex> mu(*mem_maps_lock_);
  CHECK(begin_map.IsValid());
  CHECK(end_map.IsValid());
  CHECK(HasMemMap(begin_map));
  CHECK(HasMemMap(end_map));
  CHECK_LE(begin_map.BaseBegin(), end_map.BaseBegin());
  MemMap* map = &begin_map;
  while (map->BaseBegin() != end_map.BaseBegin()) {
    MemMap* next_map = GetLargestMemMapAt(map->BaseEnd());
    if (next_map == nullptr) {
      // Found a gap.
      return false;
    }
    map = next_map;
  }
  return true;
}

void MemMap::DumpMaps(std::ostream& os, bool terse) {
  std::lock_guard<std::mutex> mu(*mem_maps_lock_);
  DumpMapsLocked(os, terse);
}

void MemMap::DumpMapsLocked(std::ostream& os, bool terse) {
  const auto& mem_maps = *gMaps;
  if (!terse) {
    os << mem_maps;
    return;
  }

  // Terse output example:
  //   [MemMap: 0x409be000+0x20P~0x11dP+0x20P~0x61cP+0x20P prot=0x3 LinearAlloc]
  //   [MemMap: 0x451d6000+0x6bP(3) prot=0x3 large object space allocation]
  // The details:
  //   "+0x20P" means 0x20 pages taken by a single mapping,
  //   "~0x11dP" means a gap of 0x11d pages,
  //   "+0x6bP(3)" means 3 mappings one after another, together taking 0x6b pages.
  os << "MemMap:" << std::endl;
  for (auto it = mem_maps.begin(), maps_end = mem_maps.end(); it != maps_end;) {
    MemMap* map = it->second;
    void* base = it->first;
    CHECK_EQ(base, map->BaseBegin());
    os << "[MemMap: " << base;
    ++it;
    // Merge consecutive maps with the same protect flags and name.
    constexpr size_t kMaxGaps = 9;
    size_t num_gaps = 0;
    size_t num = 1u;
    size_t size = map->BaseSize();
    CHECK_ALIGNED(size, kPageSize);
    void* end = map->BaseEnd();
    while (it != maps_end &&
        it->second->GetProtect() == map->GetProtect() &&
        it->second->GetName() == map->GetName() &&
        (it->second->BaseBegin() == end || num_gaps < kMaxGaps)) {
      if (it->second->BaseBegin() != end) {
        ++num_gaps;
        os << "+0x" << std::hex << (size / kPageSize) << "P";
        if (num != 1u) {
          os << "(" << std::dec << num << ")";
        }
        size_t gap =
            reinterpret_cast<uintptr_t>(it->second->BaseBegin()) - reinterpret_cast<uintptr_t>(end);
        CHECK_ALIGNED(gap, kPageSize);
        os << "~0x" << std::hex << (gap / kPageSize) << "P";
        num = 0u;
        size = 0u;
      }
      CHECK_ALIGNED(it->second->BaseSize(), kPageSize);
      ++num;
      size += it->second->BaseSize();
      end = it->second->BaseEnd();
      ++it;
    }
    os << "+0x" << std::hex << (size / kPageSize) << "P";
    if (num != 1u) {
      os << "(" << std::dec << num << ")";
    }
    os << " prot=0x" << std::hex << map->GetProtect() << " " << map->GetName() << "]" << std::endl;
  }
}

bool MemMap::HasMemMap(MemMap& map) {
  void* base_begin = map.BaseBegin();
  for (auto it = gMaps->lower_bound(base_begin), end = gMaps->end();
       it != end && it->first == base_begin; ++it) {
    if (it->second == &map) {
      return true;
    }
  }
  return false;
}

MemMap* MemMap::GetLargestMemMapAt(void* address) {
  size_t largest_size = 0;
  MemMap* largest_map = nullptr;
  DCHECK(gMaps != nullptr);
  for (auto it = gMaps->lower_bound(address), end = gMaps->end();
       it != end && it->first == address; ++it) {
    MemMap* map = it->second;
    CHECK(map != nullptr);
    if (largest_size < map->BaseSize()) {
      largest_size = map->BaseSize();
      largest_map = map;
    }
  }
  return largest_map;
}

void MemMap::Init() {
  if (mem_maps_lock_ != nullptr) {
    // dex2oat calls MemMap::Init twice since its needed before the runtime is created.
    return;
  }
  mem_maps_lock_ = new std::mutex();
  // Not for thread safety, but for the annotation that gMaps is GUARDED_BY(mem_maps_lock_).
  std::lock_guard<std::mutex> mu(*mem_maps_lock_);
  DCHECK(gMaps == nullptr);
  gMaps = new Maps;

  TargetMMapInit();
}

void MemMap::Shutdown() {
  if (mem_maps_lock_ == nullptr) {
    // If MemMap::Shutdown is called more than once, there is no effect.
    return;
  }
  {
    // Not for thread safety, but for the annotation that gMaps is GUARDED_BY(mem_maps_lock_).
    std::lock_guard<std::mutex> mu(*mem_maps_lock_);
    DCHECK(gMaps != nullptr);
    delete gMaps;
    gMaps = nullptr;
  }
  delete mem_maps_lock_;
  mem_maps_lock_ = nullptr;
}

void MemMap::SetSize(size_t new_size) {
  CHECK_LE(new_size, size_);
  size_t new_base_size = RoundUp(new_size + static_cast<size_t>(PointerDiff(Begin(), BaseBegin())),
                                 kPageSize);
  if (new_base_size == base_size_) {
    size_ = new_size;
    return;
  }
  CHECK_LT(new_base_size, base_size_);
  MEMORY_TOOL_MAKE_UNDEFINED(
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(BaseBegin()) +
                              new_base_size),
      base_size_ - new_base_size);
  CHECK_EQ(TargetMUnmap(reinterpret_cast<void*>(
                        reinterpret_cast<uintptr_t>(BaseBegin()) + new_base_size),
                        base_size_ - new_base_size), 0)
                        << new_base_size << " " << base_size_;
  base_size_ = new_base_size;
  size_ = new_size;
}

void* MemMap::MapInternalArtLow4GBAllocator(size_t length,
                                            int prot,
                                            int flags,
                                            int fd,
                                            off_t offset) {
#if USE_ART_LOW_4G_ALLOCATOR
  void* actual = MAP_FAILED;

  bool first_run = true;

  std::lock_guard<std::mutex> mu(*mem_maps_lock_);
  for (uintptr_t ptr = next_mem_pos_; ptr < 4 * GB; ptr += kPageSize) {
    // Use gMaps as an optimization to skip over large maps.
    // Find the first map which is address > ptr.
    auto it = gMaps->upper_bound(reinterpret_cast<void*>(ptr));
    if (it != gMaps->begin()) {
      auto before_it = it;
      --before_it;
      // Start at the end of the map before the upper bound.
      ptr = std::max(ptr, reinterpret_cast<uintptr_t>(before_it->second->BaseEnd()));
      CHECK_ALIGNED(ptr, kPageSize);
    }
    while (it != gMaps->end()) {
      // How much space do we have until the next map?
      size_t delta = reinterpret_cast<uintptr_t>(it->first) - ptr;
      // If the space may be sufficient, break out of the loop.
      if (delta >= length) {
        break;
      }
      // Otherwise, skip to the end of the map.
      ptr = reinterpret_cast<uintptr_t>(it->second->BaseEnd());
      CHECK_ALIGNED(ptr, kPageSize);
      ++it;
    }

    // Try to see if we get lucky with this address since none of the ART maps overlap.
    actual = TryMemMapLow4GB(reinterpret_cast<void*>(ptr), length, prot, flags, fd, offset);
    if (actual != MAP_FAILED) {
      next_mem_pos_ = reinterpret_cast<uintptr_t>(actual) + length;
      return actual;
    }

    if (4U * GB - ptr < length) {
      // Not enough memory until 4GB.
      if (first_run) {
        // Try another time from the bottom;
        ptr = LOW_MEM_START - kPageSize;
        first_run = false;
        continue;
      } else {
        // Second try failed.
        break;
      }
    }

    uintptr_t tail_ptr;

    // Check pages are free.
    bool safe = true;
    for (tail_ptr = ptr; tail_ptr < ptr + length; tail_ptr += kPageSize) {
      if (msync(reinterpret_cast<void*>(tail_ptr), kPageSize, 0) == 0) {
        safe = false;
        break;
      } else {
        DCHECK_EQ(errno, ENOMEM);
      }
    }

    next_mem_pos_ = tail_ptr;  // update early, as we break out when we found and mapped a region

    if (safe == true) {
      actual = TryMemMapLow4GB(reinterpret_cast<void*>(ptr), length, prot, flags, fd, offset);
      if (actual != MAP_FAILED) {
        return actual;
      }
    } else {
      // Skip over last page.
      ptr = tail_ptr;
    }
  }

  if (actual == MAP_FAILED) {
    LOG(ERROR) << "Could not find contiguous low-memory space.";
    errno = ENOMEM;
  }
  return actual;
#else
  UNUSED(length, prot, flags, fd, offset);
  LOG(FATAL) << "Unreachable";
  UNREACHABLE();
#endif
}

void* MemMap::MapInternal(void* addr,
                          size_t length,
                          int prot,
                          int flags,
                          int fd,
                          off_t offset,
                          bool low_4gb) {
#ifdef __LP64__
  // When requesting low_4g memory and having an expectation, the requested range should fit into
  // 4GB.
  if (low_4gb && (
      // Start out of bounds.
      (reinterpret_cast<uintptr_t>(addr) >> 32) != 0 ||
      // End out of bounds. For simplicity, this will fail for the last page of memory.
      ((reinterpret_cast<uintptr_t>(addr) + length) >> 32) != 0)) {
    LOG(ERROR) << "The requested address space (" << addr << ", "
               << reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(addr) + length)
               << ") cannot fit in low_4gb";
    return MAP_FAILED;
  }
#else
  UNUSED(low_4gb);
#endif
  DCHECK_ALIGNED(length, kPageSize);
  // TODO:
  // A page allocator would be a useful abstraction here, as
  // 1) It is doubtful that MAP_32BIT on x86_64 is doing the right job for us
  void* actual = MAP_FAILED;
#if USE_ART_LOW_4G_ALLOCATOR
  // MAP_32BIT only available on x86_64.
  if (low_4gb && addr == nullptr) {
    // The linear-scan allocator has an issue when executable pages are denied (e.g., by selinux
    // policies in sensitive processes). In that case, the error code will still be ENOMEM. So
    // the allocator will scan all low 4GB twice, and still fail. This is *very* slow.
    //
    // To avoid the issue, always map non-executable first, and mprotect if necessary.
    const int orig_prot = prot;
    const int prot_non_exec = prot & ~PROT_EXEC;
    actual = MapInternalArtLow4GBAllocator(length, prot_non_exec, flags, fd, offset);

    if (actual == MAP_FAILED) {
      return MAP_FAILED;
    }

    // See if we need to remap with the executable bit now.
    if (orig_prot != prot_non_exec) {
      if (mprotect(actual, length, orig_prot) != 0) {
        PLOG(ERROR) << "Could not protect to requested prot: " << orig_prot;
        TargetMUnmap(actual, length);
        errno = ENOMEM;
        return MAP_FAILED;
      }
    }
    return actual;
  }

  actual = TargetMMap(addr, length, prot, flags, fd, offset);
#else
#if defined(__LP64__)
  if (low_4gb && addr == nullptr) {
    flags |= MAP_32BIT;
  }
#endif
  actual = TargetMMap(addr, length, prot, flags, fd, offset);
#endif
  return actual;
}

std::ostream& operator<<(std::ostream& os, const MemMap& mem_map) {
  os << StringPrintf("[MemMap: %p-%p prot=0x%x %s]",
                     mem_map.BaseBegin(), mem_map.BaseEnd(), mem_map.GetProtect(),
                     mem_map.GetName().c_str());
  return os;
}

void MemMap::TryReadable() {
  if (base_begin_ == nullptr && base_size_ == 0) {
    return;
  }
  CHECK_NE(prot_ & PROT_READ, 0);
  volatile uint8_t* begin = reinterpret_cast<volatile uint8_t*>(base_begin_);
  volatile uint8_t* end = begin + base_size_;
  DCHECK(IsAligned<kPageSize>(begin));
  DCHECK(IsAligned<kPageSize>(end));
  // Read the first byte of each page. Use volatile to prevent the compiler from optimizing away the
  // reads.
  for (volatile uint8_t* ptr = begin; ptr < end; ptr += kPageSize) {
    // This read could fault if protection wasn't set correctly.
    uint8_t value = *ptr;
    UNUSED(value);
  }
}

void ZeroAndReleasePages(void* address, size_t length) {
  if (length == 0) {
    return;
  }
  uint8_t* const mem_begin = reinterpret_cast<uint8_t*>(address);
  uint8_t* const mem_end = mem_begin + length;
  uint8_t* const page_begin = AlignUp(mem_begin, kPageSize);
  uint8_t* const page_end = AlignDown(mem_end, kPageSize);
  if (!kMadviseZeroes || page_begin >= page_end) {
    // No possible area to madvise.
    std::fill(mem_begin, mem_end, 0);
  } else {
    // Spans one or more pages.
    DCHECK_LE(mem_begin, page_begin);
    DCHECK_LE(page_begin, page_end);
    DCHECK_LE(page_end, mem_end);
    std::fill(mem_begin, page_begin, 0);
#ifdef _WIN32
    LOG(WARNING) << "ZeroAndReleasePages does not madvise on Windows.";
#else
    CHECK_NE(madvise(page_begin, page_end - page_begin, MADV_DONTNEED), -1) << "madvise failed";
#endif
    std::fill(page_end, mem_end, 0);
  }
}

void MemMap::AlignBy(size_t size) {
  CHECK_EQ(begin_, base_begin_) << "Unsupported";
  CHECK_EQ(size_, base_size_) << "Unsupported";
  CHECK_GT(size, static_cast<size_t>(kPageSize));
  CHECK_ALIGNED(size, kPageSize);
  CHECK(!reuse_);
  if (IsAlignedParam(reinterpret_cast<uintptr_t>(base_begin_), size) &&
      IsAlignedParam(base_size_, size)) {
    // Already aligned.
    return;
  }
  uint8_t* base_begin = reinterpret_cast<uint8_t*>(base_begin_);
  uint8_t* base_end = base_begin + base_size_;
  uint8_t* aligned_base_begin = AlignUp(base_begin, size);
  uint8_t* aligned_base_end = AlignDown(base_end, size);
  CHECK_LE(base_begin, aligned_base_begin);
  CHECK_LE(aligned_base_end, base_end);
  size_t aligned_base_size = aligned_base_end - aligned_base_begin;
  CHECK_LT(aligned_base_begin, aligned_base_end)
      << "base_begin = " << reinterpret_cast<void*>(base_begin)
      << " base_end = " << reinterpret_cast<void*>(base_end);
  CHECK_GE(aligned_base_size, size);
  // Unmap the unaligned parts.
  if (base_begin < aligned_base_begin) {
    MEMORY_TOOL_MAKE_UNDEFINED(base_begin, aligned_base_begin - base_begin);
    CHECK_EQ(TargetMUnmap(base_begin, aligned_base_begin - base_begin), 0)
        << "base_begin=" << reinterpret_cast<void*>(base_begin)
        << " aligned_base_begin=" << reinterpret_cast<void*>(aligned_base_begin);
  }
  if (aligned_base_end < base_end) {
    MEMORY_TOOL_MAKE_UNDEFINED(aligned_base_end, base_end - aligned_base_end);
    CHECK_EQ(TargetMUnmap(aligned_base_end, base_end - aligned_base_end), 0)
        << "base_end=" << reinterpret_cast<void*>(base_end)
        << " aligned_base_end=" << reinterpret_cast<void*>(aligned_base_end);
  }
  std::lock_guard<std::mutex> mu(*mem_maps_lock_);
  if (base_begin < aligned_base_begin) {
    auto it = GetGMapsEntry(*this);
    auto node = gMaps->extract(it);
    node.key() = aligned_base_begin;
    gMaps->insert(std::move(node));
  }
  base_begin_ = aligned_base_begin;
  base_size_ = aligned_base_size;
  begin_ = aligned_base_begin;
  size_ = aligned_base_size;
  DCHECK(gMaps != nullptr);
}

}  // namespace art
