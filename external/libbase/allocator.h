/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_LIBARTBASE_BASE_ALLOCATOR_H_
#define ART_LIBARTBASE_BASE_ALLOCATOR_H_

#include <type_traits>

#include "atomic.h"
#include "macros.h"

namespace base {

static constexpr bool kEnableTrackingAllocator = false;

class Allocator {
 public:
  static Allocator* GetMallocAllocator();
  static Allocator* GetNoopAllocator();

  Allocator() {}
  virtual ~Allocator() {}

  virtual void* Alloc(size_t) = 0;
  virtual void Free(void*) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Allocator);
};

// Used by TrackedAllocators.
enum AllocatorTag {
  kAllocatorTagCount,  // Must always be last element.
    kAllocatorTagMaps,
};
std::ostream& operator<<(std::ostream& os, const AllocatorTag& tag);

namespace TrackedAllocators {

// Running count of number of bytes used for this kind of allocation. Increased by allocations,
// decreased by deallocations.
extern Atomic<size_t> g_bytes_used[kAllocatorTagCount];

// Largest value of bytes used seen.
extern volatile size_t g_max_bytes_used[kAllocatorTagCount];

// Total number of bytes allocated of this kind.
extern Atomic<uint64_t> g_total_bytes_used[kAllocatorTagCount];

void Dump(std::ostream& os);

inline void RegisterAllocation(AllocatorTag tag, size_t bytes) {
  g_total_bytes_used[tag].FetchAndAddSequentiallyConsistent(bytes);
  size_t new_bytes = g_bytes_used[tag].FetchAndAddSequentiallyConsistent(bytes) + bytes;
  if (g_max_bytes_used[tag] < new_bytes) {
    g_max_bytes_used[tag] = new_bytes;
  }
}

inline void RegisterFree(AllocatorTag tag, size_t bytes) {
  g_bytes_used[tag].FetchAndSubSequentiallyConsistent(bytes);
}

}  // namespace TrackedAllocators

// Tracking allocator for use with STL types, tracks how much memory is used.
template<class T, AllocatorTag kTag>
class TrackingAllocatorImpl : public std::allocator<T> {
 public:
  typedef typename std::allocator<T>::value_type value_type;
  typedef typename std::allocator<T>::size_type size_type;
  typedef typename std::allocator<T>::difference_type difference_type;
  typedef typename std::allocator<T>::pointer pointer;
  typedef typename std::allocator<T>::const_pointer const_pointer;
  typedef typename std::allocator<T>::reference reference;
  typedef typename std::allocator<T>::const_reference const_reference;

  // Used internally by STL data structures.
  template <class U>
  TrackingAllocatorImpl(
      const TrackingAllocatorImpl<U, kTag>& alloc ATTRIBUTE_UNUSED) noexcept {}

  // Used internally by STL data structures.
  TrackingAllocatorImpl() noexcept {
    static_assert(kTag < kAllocatorTagCount, "kTag must be less than kAllocatorTagCount");
  }

  // Enables an allocator for objects of one type to allocate storage for objects of another type.
  // Used internally by STL data structures.
  template <class U>
  struct rebind {
    typedef TrackingAllocatorImpl<U, kTag> other;
  };

  pointer allocate(size_type n, const_pointer hint ATTRIBUTE_UNUSED = 0) {
    const size_t size = n * sizeof(T);
    TrackedAllocators::RegisterAllocation(GetTag(), size);
    return reinterpret_cast<pointer>(malloc(size));
  }

  template <typename PT>
  void deallocate(PT p, size_type n) {
    const size_t size = n * sizeof(T);
    TrackedAllocators::RegisterFree(GetTag(), size);
    free(p);
  }

  static constexpr AllocatorTag GetTag() {
    return kTag;
  }
};

template<class T, AllocatorTag kTag>
// C++ doesn't allow template typedefs. This is a workaround template typedef which is
// TrackingAllocatorImpl<T> if kEnableTrackingAllocator is true, std::allocator<T> otherwise.
using TrackingAllocator = typename std::conditional<kEnableTrackingAllocator,
                                                    TrackingAllocatorImpl<T, kTag>,
                                                    std::allocator<T>>::type;

}  // namespace base

#endif  // ART_LIBARTBASE_BASE_ALLOCATOR_H_
