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

#include "allocator.h"

#include <inttypes.h>
#include <stdlib.h>

#include "logging.h"

#include "atomic.h"

namespace base {

    class MallocAllocator FINAL : public Allocator {
    public:
        MallocAllocator() {}

        ~MallocAllocator() {}

        void *Alloc(size_t size) {
            return calloc(sizeof(uint8_t), size);
        }

        void Free(void *p) {
            free(p);
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(MallocAllocator);
    };

    MallocAllocator g_malloc_allocator;

    class NoopAllocator FINAL : public Allocator {
    public:
        NoopAllocator() {}

        ~NoopAllocator() {}

        void *Alloc(size_t size ATTRIBUTE_UNUSED) {
            LOG(FATAL) << "NoopAllocator::Alloc should not be called";
            UNREACHABLE();
        }

        void Free(void *p ATTRIBUTE_UNUSED) {
            // Noop.
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(NoopAllocator);
    };

    NoopAllocator g_noop_allocator;

    Allocator *Allocator::GetMallocAllocator() {
        return &g_malloc_allocator;
    }

    Allocator *Allocator::GetNoopAllocator() {
        return &g_noop_allocator;
    }

    namespace TrackedAllocators {

// These globals are safe since they don't have any non-trivial destructors.
        Atomic<size_t> g_bytes_used[kAllocatorTagCount];
        volatile size_t g_max_bytes_used[kAllocatorTagCount];
        Atomic<uint64_t> g_total_bytes_used[kAllocatorTagCount];

        void Dump(std::ostream &os) {

        }

    }  // namespace TrackedAllocators

}  // namespace base
