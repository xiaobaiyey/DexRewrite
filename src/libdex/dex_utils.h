//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_DEX_UTILS_H
#define WAAPOLICY_DEX_UTILS_H


#include <stdio.h>

#include <string>

inline void StringAppendV(std::string *dst, const char *format, va_list ap) {
    // First try with a small fixed size buffer
    char space[1024];

    // It's possible for methods that use a va_list to invalidate
    // the data in it upon use.  The fix is to make a copy
    // of the structure before using it and use that copy instead.
    va_list backup_ap;
    va_copy(backup_ap, ap);
    int result = vsnprintf(space, sizeof(space), format, backup_ap);
    va_end(backup_ap);

    if (result < static_cast<int>(sizeof(space))) {
        if (result >= 0) {
            // Normal case -- everything fit.
            dst->append(space, result);
            return;
        }

        if (result < 0) {
            // Just an error.
            return;
        }
    }

    // Increase the buffer size to the size requested by vsnprintf,
    // plus one for the closing \0.
    int length = result + 1;
    char *buf = new char[length];

    // Restore the va_list before we use it again
    va_copy(backup_ap, ap);
    result = vsnprintf(buf, length, format, backup_ap);
    va_end(backup_ap);

    if (result >= 0 && result < length) {
        // It fit
        dst->append(buf, result);
    }
    delete[] buf;
}

inline std::string StringPrintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string result;
    StringAppendV(&result, fmt, ap);
    va_end(ap);
    return result;
}

inline void StringAppendF(std::string *dst, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    StringAppendV(dst, format, ap);
    va_end(ap);
}


inline uint16_t GetTrailingUtf16Char(uint32_t maybe_pair) {
    return static_cast<uint16_t>(maybe_pair >> 16);
}

inline uint16_t GetLeadingUtf16Char(uint32_t maybe_pair) {
    return static_cast<uint16_t>(maybe_pair & 0x0000FFFF);
}

inline uint32_t GetUtf16FromUtf8(const char **utf8_data_in) {
    const uint8_t one = *(*utf8_data_in)++;
    if ((one & 0x80) == 0) {
        // one-byte encoding
        return one;
    }

    const uint8_t two = *(*utf8_data_in)++;
    if ((one & 0x20) == 0) {
        // two-byte encoding
        return ((one & 0x1f) << 6) | (two & 0x3f);
    }

    const uint8_t three = *(*utf8_data_in)++;
    if ((one & 0x10) == 0) {
        return ((one & 0x0f) << 12) | ((two & 0x3f) << 6) | (three & 0x3f);
    }

    // Four byte encodings need special handling. We'll have
    // to convert them into a surrogate pair.
    const uint8_t four = *(*utf8_data_in)++;

    // Since this is a 4 byte UTF-8 sequence, it will lie between
    // U+10000 and U+1FFFFF.
    //
    // TODO: What do we do about values in (U+10FFFF, U+1FFFFF) ? The
    // spec says they're invalid but nobody appears to check for them.
    const uint32_t code_point = ((one & 0x0f) << 18) | ((two & 0x3f) << 12)
                                | ((three & 0x3f) << 6) | (four & 0x3f);

    uint32_t surrogate_pair = 0;
    // Step two: Write out the high (leading) surrogate to the bottom 16 bits
    // of the of the 32 bit type.
    surrogate_pair |= ((code_point >> 10) + 0xd7c0) & 0xffff;
    // Step three : Write out the low (trailing) surrogate to the top 16 bits.
    surrogate_pair |= ((code_point & 0x03ff) + 0xdc00) << 16;

    return surrogate_pair;
}

inline int CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(const char *utf8_1,
                                                                   const char *utf8_2) {
    uint32_t c1, c2;
    do {
        c1 = *utf8_1;
        c2 = *utf8_2;
        // Did we reach a terminating character?
        if (c1 == 0) {
            return (c2 == 0) ? 0 : -1;
        } else if (c2 == 0) {
            return 1;
        }

        c1 = GetUtf16FromUtf8(&utf8_1);
        c2 = GetUtf16FromUtf8(&utf8_2);
    } while (c1 == c2);

    const uint32_t leading_surrogate_diff = GetLeadingUtf16Char(c1) - GetLeadingUtf16Char(c2);
    if (leading_surrogate_diff != 0) {
        return static_cast<int>(leading_surrogate_diff);
    }

    return GetTrailingUtf16Char(c1) - GetTrailingUtf16Char(c2);
}

inline int CompareModifiedUtf8ToUtf16AsCodePointValues(const char *utf8, const uint16_t *utf16,
                                                       size_t utf16_length) {
    for (;;) {
        if (*utf8 == '\0') {
            return (utf16_length == 0) ? 0 : -1;
        } else if (utf16_length == 0) {
            return 1;
        }

        const uint32_t pair = GetUtf16FromUtf8(&utf8);

        // First compare the leading utf16 char.
        const uint16_t lhs = GetLeadingUtf16Char(pair);
        const uint16_t rhs = *utf16++;
        --utf16_length;
        if (lhs != rhs) {
            return lhs > rhs ? 1 : -1;
        }

        // Then compare the trailing utf16 char. First check if there
        // are any characters left to consume.
        const uint16_t lhs2 = GetTrailingUtf16Char(pair);
        if (lhs2 != 0) {
            if (utf16_length == 0) {
                return 1;
            }

            const uint16_t rhs2 = *utf16++;
            --utf16_length;
            if (lhs2 != rhs2) {
                return lhs2 > rhs2 ? 1 : -1;
            }
        }
    }
}

inline size_t CountModifiedUtf8Chars(const char *utf8, size_t byte_count) {
    DCHECK_LE(byte_count, strlen(utf8));
    size_t len = 0;
    const char *end = utf8 + byte_count;
    for (; utf8 < end; ++utf8) {
        int ic = *utf8;
        len++;
        if (LIKELY((ic & 0x80) == 0)) {
            // One-byte encoding.
            continue;
        }
        // Two- or three-byte encoding.
        utf8++;
        if ((ic & 0x20) == 0) {
            // Two-byte encoding.
            continue;
        }
        utf8++;
        if ((ic & 0x10) == 0) {
            // Three-byte encoding.
            continue;
        }

        // Four-byte encoding: needs to be converted into a surrogate
        // pair.
        utf8++;
        len++;
    }
    return len;
}

static inline constexpr bool NeedsEscaping(uint16_t ch) {
    return (ch < ' ' || ch > '~');
}

inline size_t CountModifiedUtf8Chars(const char *utf8) {
    return CountModifiedUtf8Chars(utf8, strlen(utf8));
}

inline std::string PrintableString(const char *utf) {
    std::string result;
    result += '"';
    const char *p = utf;
    size_t char_count = CountModifiedUtf8Chars(p);
    for (size_t i = 0; i < char_count; ++i) {
        uint32_t ch = GetUtf16FromUtf8(&p);
        if (ch == '\\') {
            result += "\\\\";
        } else if (ch == '\n') {
            result += "\\n";
        } else if (ch == '\r') {
            result += "\\r";
        } else if (ch == '\t') {
            result += "\\t";
        } else {
            const uint16_t leading = GetLeadingUtf16Char(ch);

            if (NeedsEscaping(leading)) {
                StringAppendF(&result, "\\u%04x", leading);
            } else {
                result += static_cast<std::string::value_type>(leading);
            }

            const uint32_t trailing = GetTrailingUtf16Char(ch);
            if (trailing != 0) {
                // All high surrogates will need escaping.
                StringAppendF(&result, "\\u%04x", trailing);
            }
        }
    }
    result += '"';
    return result;
}


inline void AppendPrettyDescriptor(const char* descriptor, std::string* result) {
    // Count the number of '['s to get the dimensionality.
    const char* c = descriptor;
    size_t dim = 0;
    while (*c == '[') {
        dim++;
        c++;
    }

    // Reference or primitive?
    if (*c == 'L') {
        // "[[La/b/C;" -> "a.b.C[][]".
        c++;  // Skip the 'L'.
    } else {
        // "[[B" -> "byte[][]".
        // To make life easier, we make primitives look like unqualified
        // reference types.
        switch (*c) {
            case 'B': c = "byte;"; break;
            case 'C': c = "char;"; break;
            case 'D': c = "double;"; break;
            case 'F': c = "float;"; break;
            case 'I': c = "int;"; break;
            case 'J': c = "long;"; break;
            case 'S': c = "short;"; break;
            case 'Z': c = "boolean;"; break;
            case 'V': c = "void;"; break;  // Used when decoding return types.
            default: result->append(descriptor); return;
        }
    }

    // At this point, 'c' is a string of the form "fully/qualified/Type;"
    // or "primitive;". Rewrite the type with '.' instead of '/':
    const char* p = c;
    while (*p != ';') {
        char ch = *p++;
        if (ch == '/') {
            ch = '.';
        }
        result->push_back(ch);
    }
    // ...and replace the semicolon with 'dim' "[]" pairs:
    for (size_t i = 0; i < dim; ++i) {
        result->append("[]");
    }
}

inline std::string PrettyDescriptor(const char* descriptor) {
    std::string result;
    AppendPrettyDescriptor(descriptor, &result);
    return result;
}

#endif //WAAPOLICY_DEX_UTILS_H
