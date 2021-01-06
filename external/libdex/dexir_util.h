//
// Created by xiaob on 2020/1/1.
//

#ifndef BASE_DEXIR_UTIL_H
#define BASE_DEXIR_UTIL_H
namespace dex_ir {
    // Item size constants.
    static constexpr size_t kHeaderItemSize = 112;
    static constexpr size_t kStringIdItemSize = 4;
    static constexpr size_t kTypeIdItemSize = 4;
    static constexpr size_t kProtoIdItemSize = 12;
    static constexpr size_t kFieldIdItemSize = 8;
    static constexpr size_t kMethodIdItemSize = 8;
    static constexpr size_t kClassDefItemSize = 32;
    static constexpr size_t kCallSiteIdItemSize = 4;
    static constexpr size_t kMethodHandleItemSize = 8;

    template<class T>
    class Iterator : public std::iterator<std::random_access_iterator_tag, T> {
    public:
        using value_type = typename std::iterator<std::random_access_iterator_tag, T>::value_type;
        using difference_type =
        typename std::iterator<std::random_access_iterator_tag, value_type>::difference_type;
        using pointer = typename std::iterator<std::random_access_iterator_tag, value_type>::pointer;
        using reference = typename std::iterator<std::random_access_iterator_tag, value_type>::reference;

        Iterator(const Iterator &) = default;

        Iterator(Iterator &&) = default;

        Iterator &operator=(const Iterator &) = default;

        Iterator &operator=(Iterator &&) = default;

        Iterator(const std::vector <T> &vector,
                 uint32_t position,
                 uint32_t iterator_end)
                : vector_(&vector),
                  position_(position),
                  iterator_end_(iterator_end) {}

        Iterator() : vector_(nullptr), position_(0U), iterator_end_(0U) {}

        bool IsValid() const { return position_ < iterator_end_; }

        bool operator==(const Iterator &rhs) const { return position_ == rhs.position_; }

        bool operator!=(const Iterator &rhs) const { return !(*this == rhs); }

        bool operator<(const Iterator &rhs) const { return position_ < rhs.position_; }

        bool operator>(const Iterator &rhs) const { return rhs < *this; }

        bool operator<=(const Iterator &rhs) const { return !(rhs < *this); }

        bool operator>=(const Iterator &rhs) const { return !(*this < rhs); }

        Iterator &operator++() {  // Value after modification.
            ++position_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++position_;
            return temp;
        }

        Iterator &operator+=(difference_type delta) {
            position_ += delta;
            return *this;
        }

        Iterator operator+(difference_type delta) const {
            Iterator temp = *this;
            temp += delta;
            return temp;
        }

        Iterator &operator--() {  // Value after modification.
            --position_;
            return *this;
        }

        Iterator operator--(int) {
            Iterator temp = *this;
            --position_;
            return temp;
        }

        Iterator &operator-=(difference_type delta) {
            position_ -= delta;
            return *this;
        }

        Iterator operator-(difference_type delta) const {
            Iterator temp = *this;
            temp -= delta;
            return temp;
        }

        difference_type operator-(const Iterator &rhs) {
            return position_ - rhs.position_;
        }

        reference operator*() const {
            return const_cast<reference>((*vector_)[position_]);
        }

        pointer operator->() const {
            return const_cast<pointer>(&((*vector_)[position_]));
        }

        reference operator[](difference_type n) const {
            return (*vector_)[position_ + n];
        }

    private:
        const std::vector <T> *vector_;
        uint32_t position_;
        uint32_t iterator_end_;

        template<typename U>
        friend bool operator<(const Iterator<U> &lhs, const Iterator<U> &rhs);
    };
}
#endif //BASE_DEXIR_UTIL_H
