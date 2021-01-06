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

#ifndef ART_LIBARTBASE_BASE_HIDDENAPI_FLAGS_H_
#define ART_LIBARTBASE_BASE_HIDDENAPI_FLAGS_H_

#include "sdk_version.h"

#include <vector>

#include "libbase/logging.h"
#include "libbase/bit_utils.h"
#include "libbase/dumpable.h"
#include "libbase/macros.h"
#include "hiddenapi_stubs.h"

namespace libdex {
    namespace hiddenapi {

// Helper methods used inside ApiList. These were moved outside of the ApiList
// class so that they can be used in static_asserts. If they were inside, they
// would be part of an unfinished type.
        namespace helper {
            // Casts enum value to uint32_t.
            template<typename T>
            constexpr uint32_t ToUint(T val) { return static_cast<uint32_t>(val); }

            // Returns uint32_t with one bit set at an index given by an enum value.
            template<typename T>
            constexpr uint32_t ToBit(T val) { return 1u << ToUint(val); }

            // Returns a bit mask with `size` least significant bits set.
            constexpr uint32_t BitMask(uint32_t size) { return (1u << size) - 1; }

            // Returns a bit mask formed from an enum defining kMin and kMax. The values
            // are assumed to be indices of min/max bits and the resulting bitmask has
            // bits [kMin, kMax] set.
            template<typename T>
            constexpr uint32_t BitMask() {
                return BitMask(ToUint(T::kMax) + 1) & (~BitMask(ToUint(T::kMin)));
            }

            // Returns true if `val` is a bitwise subset of `mask`.
            constexpr bool MatchesBitMask(uint32_t val, uint32_t mask) { return (val & mask) == val; }

            // Returns true if the uint32_t value of `val` is a bitwise subset of `mask`.
            template<typename T>
            constexpr bool MatchesBitMask(T val, uint32_t mask) { return MatchesBitMask(ToUint(val), mask); }

            // Returns the number of values defined in an enum, assuming the enum defines
            // kMin and kMax and no integer values are skipped between them.
            template<typename T>
            constexpr uint32_t NumValues() { return ToUint(T::kMax) - ToUint(T::kMin) + 1; }
        }  // namespace helper

/*
 * This class represents the information whether a field/method is in
 * public API (whitelist) or if it isn't, apps targeting which SDK
 * versions are allowed to access it.
 */
        class ApiList {
        public:
            // Number of bits reserved for Value in dex flags, and the corresponding bit mask.
            static constexpr uint32_t kValueBitSize = 3;
            static constexpr uint32_t kValueBitMask = helper::BitMask(kValueBitSize);

            enum class Value : uint32_t {
                // Values independent of target SDK version of app
                        kWhitelist = 0,
                kGreylist = 1,
                kBlacklist = 2,

                // Values dependent on target SDK version of app. Put these last as
                // their list will be extended in future releases.
                // The max release code implicitly includes all maintenance releases,
                // e.g. GreylistMaxO is accessible to targetSdkVersion <= 27 (O_MR1).
                        kGreylistMaxO = 3,
                kGreylistMaxP = 4,

                // Special values
                        kInvalid = (static_cast<uint32_t>(-1) & kValueBitMask),
                kMin = kWhitelist,
                kMax = kGreylistMaxP,
            };

            Value GetValue() const {
                uint32_t value = (dex_flags_ & kValueBitMask);

                // Treat all ones as invalid value
                if (value == helper::ToUint(Value::kInvalid)) {
                    return Value::kInvalid;
                } else {
                    DCHECK_GE(value, helper::ToUint(Value::kMin));
                    DCHECK_LE(value, helper::ToUint(Value::kMax));
                    return static_cast<Value>(value);
                }
            }

        private:
            // Additional bit flags after the first kValueBitSize bits in dex flags.
            // These are used for domain-specific API.
            enum class DomainApi : uint32_t {
                kCorePlatformApi = kValueBitSize,

                // Special values
                        kMin = kCorePlatformApi,
                kMax = kCorePlatformApi,
            };

            // Bit mask of all domain API flags.
            static constexpr uint32_t kDomainApiBitMask = helper::BitMask<DomainApi>();

            // Check that Values fit in the designated number of bits.
            static_assert(kValueBitSize >= base::MinimumBitsToStore(helper::ToUint(Value::kMax)),
                          "Not enough bits to store all ApiList values");

            // Sanity checks that all Values are covered by kValueBitMask.
            static_assert(helper::MatchesBitMask(Value::kMin, kValueBitMask));
            static_assert(helper::MatchesBitMask(Value::kMax, kValueBitMask));

            // Assert that Value::kInvalid is larger than the maximum Value.
            static_assert(helper::ToUint(Value::kMax) < helper::ToUint(Value::kInvalid));

            // Names corresponding to Values.
            static constexpr const char *kValueNames[] = {
                    "whitelist",
                    "greylist",
                    "blacklist",
                    "greylist-max-o",
                    "greylist-max-p",
            };

            // Names corresponding to DomainApis.
            static constexpr const char *kDomainApiNames[]{
                    "core-platform-api",
            };

            // Maximum SDK versions allowed to access ApiList of given Value.
            static constexpr SdkVersion kMaxSdkVersions[]{
                    /* whitelist */ SdkVersion::kMax,
                    /* greylist */ SdkVersion::kMax,
                    /* blacklist */ SdkVersion::kMin,
                    /* greylist-max-o */ SdkVersion::kO_MR1,
                    /* greylist-max-p */ SdkVersion::kP,
            };

            explicit ApiList(Value val, uint32_t domain_apis = 0u)
                    : dex_flags_(helper::ToUint(val) | domain_apis) {
                DCHECK(GetValue() == val);
                DCHECK_EQ(GetDomainApis(), domain_apis);
            }

            explicit ApiList(DomainApi val) : ApiList(Value::kInvalid, helper::ToBit(val)) {}


            uint32_t GetDomainApis() const { return (dex_flags_ & kDomainApiBitMask); }

            uint32_t dex_flags_;

        public:
            ApiList() : ApiList(Value::kInvalid) {}

            explicit ApiList(uint32_t dex_flags) : dex_flags_(dex_flags) {
                DCHECK_EQ(dex_flags_, (dex_flags_ & kValueBitMask) | (dex_flags_ & kDomainApiBitMask));
            }

            // Helpers for conveniently constructing ApiList instances.
            static ApiList Whitelist() { return ApiList(Value::kWhitelist); }

            static ApiList Greylist() { return ApiList(Value::kGreylist); }

            static ApiList Blacklist() { return ApiList(Value::kBlacklist); }

            static ApiList GreylistMaxO() { return ApiList(Value::kGreylistMaxO); }

            static ApiList GreylistMaxP() { return ApiList(Value::kGreylistMaxP); }

            static ApiList CorePlatformApi() { return ApiList(DomainApi::kCorePlatformApi); }

            uint32_t GetDexFlags() const { return dex_flags_; }

            uint32_t GetIntValue() const { return helper::ToUint(GetValue()) - helper::ToUint(Value::kMin); }

            // Returns the ApiList with a flag of a given name, or an empty ApiList if not matched.
            static ApiList FromName(const std::string &str) {
                for (uint32_t i = 0; i < kValueCount; ++i) {
                    if (str == kValueNames[i]) {
                        return ApiList(static_cast<Value>(i + helper::ToUint(Value::kMin)));
                    }
                }
                for (uint32_t i = 0; i < kDomainApiCount; ++i) {
                    if (str == kDomainApiNames[i]) {
                        return ApiList(static_cast<DomainApi>(i + helper::ToUint(DomainApi::kMin)));
                    }
                }
                return ApiList();
            }

            // Parses a vector of flag names into a single ApiList value. If successful,
            // returns true and assigns the new ApiList to `out_api_list`.
            static bool FromNames(std::vector<std::string>::iterator begin,
                                  std::vector<std::string>::iterator end,
                    /* out */ ApiList *out_api_list) {
                ApiList api_list;
                for (std::vector<std::string>::iterator it = begin; it != end; it++) {
                    ApiList current = FromName(*it);
                    if (current.IsEmpty() || !api_list.CanCombineWith(current)) {
                        if (ApiStubs::IsStubsFlag(*it)) {
                            // Ignore flags which correspond to the stubs from where the api
                            // originates (i.e. system-api, test-api, public-api), as they are not
                            // relevant at runtime
                            continue;
                        }
                        return false;
                    }
                    api_list |= current;
                }
                if (out_api_list != nullptr) {
                    *out_api_list = api_list;
                }
                return true;
            }

            bool operator==(const ApiList &other) const { return dex_flags_ == other.dex_flags_; }

            bool operator!=(const ApiList &other) const { return !(*this == other); }

            // Returns true if combining this ApiList with `other` will succeed.
            bool CanCombineWith(const ApiList &other) const {
                const Value val1 = GetValue();
                const Value val2 = other.GetValue();
                return (val1 == val2) || (val1 == Value::kInvalid) || (val2 == Value::kInvalid);
            }

            // Combine two ApiList instances.
            ApiList operator|(const ApiList &other) {
                // DomainApis are not mutually exclusive. Simply OR them.
                const uint32_t domain_apis = GetDomainApis() | other.GetDomainApis();

                // Values are mutually exclusive. Check if `this` and `other` have the same Value
                // or if at most one is set.
                const Value val1 = GetValue();
                const Value val2 = other.GetValue();
                if (val1 == val2) {
                    return ApiList(val1, domain_apis);
                } else if (val1 == Value::kInvalid) {
                    return ApiList(val2, domain_apis);
                } else if (val2 == Value::kInvalid) {
                    return ApiList(val1, domain_apis);
                } else {
                    LOG(FATAL) << "Invalid combination of values " <<base::Dumpable(ApiList(val1))
                               << " and " << base::Dumpable(ApiList(val2));
                    UNREACHABLE();
                }
            }

            const ApiList &operator|=(const ApiList &other) {
                (*this) = (*this) | other;
                return *this;
            }

            // Returns true if all flags set in `other` are also set in `this`.
            bool Contains(const ApiList &other) const {
                return ((other.GetValue() == Value::kInvalid) || (GetValue() == other.GetValue())) &&
                       helper::MatchesBitMask(other.GetDomainApis(), GetDomainApis());
            }

            // Returns true whether the configuration is valid for runtime use.
            bool IsValid() const { return GetValue() != Value::kInvalid; }

            // Returns true when no ApiList is specified and no domain_api flags either.
            bool IsEmpty() const { return (GetValue() == Value::kInvalid) && (GetDomainApis() == 0); }

            // Returns the maximum target SDK version allowed to access this ApiList.
            SdkVersion GetMaxAllowedSdkVersion() const { return kMaxSdkVersions[GetIntValue()]; }

            void Dump(std::ostream &os) const {
                bool is_first = true;

                if (GetValue() != Value::kInvalid) {
                    os << kValueNames[GetIntValue()];
                    is_first = false;
                }

                const uint32_t domain_apis = GetDomainApis();
                for (uint32_t i = helper::ToUint(DomainApi::kMin); i <= helper::ToUint(DomainApi::kMax); i++) {
                    if (helper::MatchesBitMask(helper::ToBit(i), domain_apis)) {
                        if (is_first) {
                            is_first = false;
                        } else {
                            os << ",";
                        }
                        os << kDomainApiNames[i];
                    }
                }

                DCHECK_EQ(IsEmpty(), is_first);
            }

            static constexpr uint32_t kValueCount = helper::NumValues<Value>();
            static constexpr uint32_t kDomainApiCount = helper::NumValues<DomainApi>();
        };

        inline std::ostream &operator<<(std::ostream &os, ApiList value) {
            value.Dump(os);
            return os;
        }

    }  // namespace hiddenapi
}  // namespace libdex


#endif  // ART_LIBARTBASE_BASE_HIDDENAPI_FLAGS_H_
