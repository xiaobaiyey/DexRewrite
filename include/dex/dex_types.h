//
// Created by xiaobaiyey on 2019-07-14.
//

#ifndef WAAPOLICY_DEX_TYPES_H
#define WAAPOLICY_DEX_TYPES_H


#include <stdlib.h>
#include <stdint.h>
#include <limits>
#include <ostream>

namespace art {
    constexpr uint32_t kDexNoIndex = 0xFFFFFFFF;

    class StringIndex {
    public:
        uint32_t index_;

        constexpr StringIndex() : index_(std::numeric_limits<decltype(index_)>::max()) {}

        explicit constexpr StringIndex(uint32_t idx) : index_(idx) {}

        bool IsValid() const {
            return index_ != std::numeric_limits<decltype(index_)>::max();
        }

        static StringIndex Invalid() {
            return StringIndex(std::numeric_limits<decltype(index_)>::max());
        }

        bool operator==(const StringIndex &other) const {
            return index_ == other.index_;
        }

        bool operator!=(const StringIndex &other) const {
            return index_ != other.index_;
        }

        bool operator<(const StringIndex &other) const {
            return index_ < other.index_;
        }

        bool operator<=(const StringIndex &other) const {
            return index_ <= other.index_;
        }

        bool operator>(const StringIndex &other) const {
            return index_ > other.index_;
        }

        bool operator>=(const StringIndex &other) const {
            return index_ >= other.index_;
        }
    };

    std::ostream &operator<<(std::ostream &os, const StringIndex &index);


    class TypeIndex {
    public:
        uint16_t index_;

        constexpr TypeIndex() : index_(std::numeric_limits<decltype(index_)>::max()) {}

        explicit constexpr TypeIndex(uint16_t idx) : index_(idx) {}

        bool IsValid() const {
            return index_ != std::numeric_limits<decltype(index_)>::max();
        }

        static TypeIndex Invalid() {
            return TypeIndex(std::numeric_limits<decltype(index_)>::max());
        }

        bool operator==(const TypeIndex &other) const {
            return index_ == other.index_;
        }

        bool operator!=(const TypeIndex &other) const {
            return index_ != other.index_;
        }

        bool operator<(const TypeIndex &other) const {
            return index_ < other.index_;
        }

        bool operator<=(const TypeIndex &other) const {
            return index_ <= other.index_;
        }

        bool operator>(const TypeIndex &other) const {
            return index_ > other.index_;
        }

        bool operator>=(const TypeIndex &other) const {
            return index_ >= other.index_;
        }
    };

    std::ostream &operator<<(std::ostream &os, const TypeIndex &index);

}


namespace std {

    template<>
    struct hash<art::StringIndex> {
        size_t operator()(const art::StringIndex &index) const {
            return hash<uint32_t>()(index.index_);
        }
    };

    template<>
    struct hash<art::TypeIndex> {
        size_t operator()(const art::TypeIndex &index) const {
            return hash<uint16_t>()(index.index_);
        }
    };

}  // namespace std

#endif //WAAPOLICY_DEX_TYPES_H
