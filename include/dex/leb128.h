/**
* @ name unpacker
* @ author xiaobaiyey
* @ email xiaobaiyey@outlook.com
* @ time 2019/4/17 5:41 PM
* @ class describe
*/

#ifndef UNPACKER_LEB128_H
#define UNPACKER_LEB128_H

#include <stdint.h>
#include <base/macros.h>
#include <base/bit_utils.h>

// Reads an unsigned LEB128 value, updating the given pointer to point
// just past the end of the read value. This function tolerates
// non-zero high-order bits in the fifth encoded byte.
static inline uint32_t DecodeUnsignedLeb128(const uint8_t **data) {
    const uint8_t *ptr = *data;
    int result = *(ptr++);
    if (UNLIKELY(result > 0x7f)) {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {
                    // Note: We don't check to see if cur is out of range here,
                    // meaning we tolerate garbage in the four high-order bits.
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *data = ptr;
    return static_cast<uint32_t>(result);
}


static inline uint32_t DecodeUnsignedLeb128WithoutMovingCursor(const uint8_t *data) {
    return DecodeUnsignedLeb128(&data);
}


static inline bool DecodeUnsignedLeb128Checked(const uint8_t **data,
                                               const void *end,
                                               uint32_t *out) {
    const uint8_t *ptr = *data;
    if (ptr >= end) {
        return false;
    }
    int result = *(ptr++);
    if (UNLIKELY(result > 0x7f)) {
        if (ptr >= end) {
            return false;
        }
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            if (ptr >= end) {
                return false;
            }
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                if (ptr >= end) {
                    return false;
                }
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {
                    if (ptr >= end) {
                        return false;
                    }
                    // Note: We don't check to see if cur is out of range here,
                    // meaning we tolerate garbage in the four high-order bits.
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *data = ptr;
    *out = static_cast<uint32_t>(result);
    return true;
}
//Reads an unsigned LEB128 + 1 value. updating the given pointer to point
// just past the end of the read value. This function tolerates
// non-zero high-order bits in the fifth encoded byte.
// It is possible for this function to return -1.
static inline int32_t DecodeUnsignedLeb128P1(const uint8_t** data) {
    return DecodeUnsignedLeb128(data) - 1;
}

// Reads a signed LEB128 value, updating the given pointer to point
// just past the end of the read value. This function tolerates
// non-zero high-order bits in the fifth encoded byte.
static inline int32_t DecodeSignedLeb128(const uint8_t** data) {
    const uint8_t* ptr = *data;
    int32_t result = *(ptr++);
    if (result <= 0x7f) {
        result = (result << 25) >> 25;
    } else {
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur <= 0x7f) {
            result = (result << 18) >> 18;
        } else {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur <= 0x7f) {
                result = (result << 11) >> 11;
            } else {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur <= 0x7f) {
                    result = (result << 4) >> 4;
                } else {
                    // Note: We don't check to see if cur is out of range here,
                    // meaning we tolerate garbage in the four high-order bits.
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *data = ptr;
    return result;
}

static inline bool DecodeSignedLeb128Checked(const uint8_t** data,
                                             const void* end,
                                             int32_t* out) {
    const uint8_t* ptr = *data;
    if (ptr >= end) {
        return false;
    }
    int32_t result = *(ptr++);
    if (result <= 0x7f) {
        result = (result << 25) >> 25;
    } else {
        if (ptr >= end) {
            return false;
        }
        int cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur <= 0x7f) {
            result = (result << 18) >> 18;
        } else {
            if (ptr >= end) {
                return false;
            }
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur <= 0x7f) {
                result = (result << 11) >> 11;
            } else {
                if (ptr >= end) {
                    return false;
                }
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur <= 0x7f) {
                    result = (result << 4) >> 4;
                } else {
                    if (ptr >= end) {
                        return false;
                    }
                    // Note: We don't check to see if cur is out of range here,
                    // meaning we tolerate garbage in the four high-order bits.
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *data = ptr;
    *out = static_cast<uint32_t>(result);
    return true;
}

// Returns the number of bytes needed to encode the value in unsigned LEB128.
static inline uint32_t UnsignedLeb128Size(uint32_t data) {
    // bits_to_encode = (data != 0) ? 32 - CLZ(x) : 1  // 32 - CLZ(data | 1)
    // bytes = ceil(bits_to_encode / 7.0);             // (6 + bits_to_encode) / 7
    uint32_t x = 6 + 32 - CLZ(data | 1U);
    // Division by 7 is done by (x * 37) >> 8 where 37 = ceil(256 / 7).
    // This works for 0 <= x < 256 / (7 * 37 - 256), i.e. 0 <= x <= 85.
    return (x * 37) >> 8;
}


static inline uint8_t* EncodeSignedLeb128(uint8_t* dest, int32_t value) {
    uint32_t extra_bits = static_cast<uint32_t>(value ^ (value >> 31)) >> 6;
    uint8_t out = value & 0x7f;
    while (extra_bits != 0u) {
        *dest++ = out | 0x80;
        value >>= 7;
        out = value & 0x7f;
        extra_bits >>= 7;
    }
    *dest++ = out;
    return dest;
}

template<typename Vector>
static inline void EncodeSignedLeb128(Vector* dest, int32_t value) {
    static_assert(std::is_same<typename Vector::value_type, uint8_t>::value, "Invalid value type");
    uint32_t extra_bits = static_cast<uint32_t>(value ^ (value >> 31)) >> 6;
    uint8_t out = value & 0x7f;
    while (extra_bits != 0u) {
        dest->push_back(out | 0x80);
        value >>= 7;
        out = value & 0x7f;
        extra_bits >>= 7;
    }
    dest->push_back(out);
}


static inline uint8_t* EncodeUnsignedLeb128(uint8_t* dest, uint32_t value) {
    uint8_t out = value & 0x7f;
    value >>= 7;
    while (value != 0) {
        *dest++ = out | 0x80;
        out = value & 0x7f;
        value >>= 7;
    }
    *dest++ = out;
    return dest;
}

template <typename Vector>
static inline void EncodeUnsignedLeb128(Vector* dest, uint32_t value) {
    static_assert(std::is_same<typename Vector::value_type, uint8_t>::value, "Invalid value type");
    uint8_t out = value & 0x7f;
    value >>= 7;
    while (value != 0) {
        dest->push_back(out | 0x80);
        out = value & 0x7f;
        value >>= 7;
    }
    dest->push_back(out);
}


#endif //UNPACKER_LEB128_H
