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

#include <libbase/logging.h>
#include "descriptors_names.h"

#include "libbase/stringprintf.h"
#include "libbase/stringuitls.h"

#include "libbase/macros.h"
#include "utf-inl.h"
#include <string.h>
namespace libdex {

using base::StringAppendF;

void AppendPrettyDescriptor(const char* descriptor, std::string* result) {
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

std::string PrettyDescriptor(const char* descriptor) {
  std::string result;
  AppendPrettyDescriptor(descriptor, &result);
  return result;
}

std::string GetJniShortName(const std::string& class_descriptor, const std::string& method) {
  // Remove the leading 'L' and trailing ';'...
  std::string class_name(class_descriptor);
  CHECK_EQ(class_name[0], 'L') << class_name;
  CHECK_EQ(class_name[class_name.size() - 1], ';') << class_name;
  class_name.erase(0, 1);
  class_name.erase(class_name.size() - 1, 1);

  std::string short_name;
  short_name += "Java_";
  short_name += MangleForJni(class_name);
  short_name += "_";
  short_name += MangleForJni(method);
  return short_name;
}

// See http://java.sun.com/j2se/1.5.0/docs/guide/jni/spec/design.html#wp615 for the full rules.
std::string MangleForJni(const std::string& s) {
  std::string result;
  size_t char_count = CountModifiedUtf8Chars(s.c_str());
  const char* cp = &s[0];
  for (size_t i = 0; i < char_count; ++i) {
    uint32_t ch = GetUtf16FromUtf8(&cp);
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
      result.push_back(ch);
    } else if (ch == '.' || ch == '/') {
      result += "_";
    } else if (ch == '_') {
      result += "_1";
    } else if (ch == ';') {
      result += "_2";
    } else if (ch == '[') {
      result += "_3";
    } else {
      const uint16_t leading = GetLeadingUtf16Char(ch);
      const uint32_t trailing = GetTrailingUtf16Char(ch);

      StringAppendF(&result, "_0%04x", leading);
      if (trailing != 0) {
        StringAppendF(&result, "_0%04x", trailing);
      }
    }
  }
  return result;
}

std::string DotToDescriptor(const char* class_name) {
  std::string descriptor(class_name);
  std::replace(descriptor.begin(), descriptor.end(), '.', '/');
  if (descriptor.length() > 0 && descriptor[0] != '[') {
    descriptor = "L" + descriptor + ";";
  }
  return descriptor;
}

std::string DescriptorToDot(const char* descriptor) {
  size_t length = strlen(descriptor);
  if (length > 1) {
    if (descriptor[0] == 'L' && descriptor[length - 1] == ';') {
      // Descriptors have the leading 'L' and trailing ';' stripped.
      std::string result(descriptor + 1, length - 2);
      std::replace(result.begin(), result.end(), '/', '.');
      return result;
    } else {
      // For arrays the 'L' and ';' remain intact.
      std::string result(descriptor);
      std::replace(result.begin(), result.end(), '/', '.');
      return result;
    }
  }
  // Do nothing for non-class/array descriptors.
  return descriptor;
}

std::string DescriptorToName(const char* descriptor) {
  size_t length = strlen(descriptor);
  if (descriptor[0] == 'L' && descriptor[length - 1] == ';') {
    std::string result(descriptor + 1, length - 2);
    return result;
  }
  return descriptor;
}

// Helper for IsValidPartOfMemberNameUtf8(), a bit vector indicating valid low ascii.
static constexpr uint32_t DEX_MEMBER_VALID_LOW_ASCII[4] = {
  0x00000000,  // 00..1f low control characters; nothing valid
  0x03ff2010,  // 20..3f digits and symbols; valid: '0'..'9', '$', '-'
  0x87fffffe,  // 40..5f uppercase etc.; valid: 'A'..'Z', '_'
  0x07fffffe   // 60..7f lowercase etc.; valid: 'a'..'z'
};

// Helper for IsValidPartOfMemberNameUtf8(); do not call directly.
COLD_ATTR
static bool IsValidPartOfMemberNameUtf8Slow(const char** pUtf8Ptr) {
  /*
   * It's a multibyte encoded character. Decode it and analyze. We
   * accept anything that isn't (a) an improperly encoded low value,
   * (b) an improper surrogate pair, (c) an encoded '\0', (d) a high
   * control character, or (e) a high space, layout, or special
   * character (U+00a0, U+2000..U+200f, U+2028..U+202f,
   * U+fff0..U+ffff). This is all specified in the dex format
   * document.
   */

  const uint32_t pair = GetUtf16FromUtf8(pUtf8Ptr);
  const uint16_t leading = GetLeadingUtf16Char(pair);

  // We have a surrogate pair resulting from a valid 4 byte UTF sequence.
  // No further checks are necessary because 4 byte sequences span code
  // points [U+10000, U+1FFFFF], which are valid codepoints in a dex
  // identifier. Furthermore, GetUtf16FromUtf8 guarantees that each of
  // the surrogate halves are valid and well formed in this instance.
  if (GetTrailingUtf16Char(pair) != 0) {
    return true;
  }


  // We've encountered a one, two or three byte UTF-8 sequence. The
  // three byte UTF-8 sequence could be one half of a surrogate pair.
  switch (leading >> 8) {
    case 0x00:
      // It's only valid if it's above the ISO-8859-1 high space (0xa0).
      return (leading > 0x00a0);
    case 0xd8:
    case 0xd9:
    case 0xda:
    case 0xdb:
      {
        // We found a three byte sequence encoding one half of a surrogate.
        // Look for the other half.
        const uint32_t pair2 = GetUtf16FromUtf8(pUtf8Ptr);
        const uint16_t trailing = GetLeadingUtf16Char(pair2);

        return (GetTrailingUtf16Char(pair2) == 0) && (0xdc00 <= trailing && trailing <= 0xdfff);
      }
    case 0xdc:
    case 0xdd:
    case 0xde:
    case 0xdf:
      // It's a trailing surrogate, which is not valid at this point.
      return false;
    case 0x20:
    case 0xff:
      // It's in the range that has spaces, controls, and specials.
      switch (leading & 0xfff8) {
        case 0x2000:
        case 0x2008:
        case 0x2028:
        case 0xfff0:
        case 0xfff8:
          return false;
      }
      return true;
    default:
      return true;
  }

  UNREACHABLE();
}

/* Return whether the pointed-at modified-UTF-8 encoded character is
 * valid as part of a member name, updating the pointer to point past
 * the consumed character. This will consume two encoded UTF-16 code
 * points if the character is encoded as a surrogate pair. Also, if
 * this function returns false, then the given pointer may only have
 * been partially advanced.
 */
ALWAYS_INLINE
static bool IsValidPartOfMemberNameUtf8(const char** pUtf8Ptr) {
  uint8_t c = (uint8_t) **pUtf8Ptr;
  if (LIKELY(c <= 0x7f)) {
    // It's low-ascii, so check the table.
    uint32_t wordIdx = c >> 5;
    uint32_t bitIdx = c & 0x1f;
    (*pUtf8Ptr)++;
    return (DEX_MEMBER_VALID_LOW_ASCII[wordIdx] & (1 << bitIdx)) != 0;
  }

  // It's a multibyte encoded character. Call a non-inline function
  // for the heavy lifting.
  return IsValidPartOfMemberNameUtf8Slow(pUtf8Ptr);
}

bool IsValidMemberName(const char* s) {
  bool angle_name = false;

  switch (*s) {
    case '\0':
      // The empty string is not a valid name.
      return false;
    case '<':
      angle_name = true;
      s++;
      break;
  }

  while (true) {
    switch (*s) {
      case '\0':
        return !angle_name;
      case '>':
        return angle_name && s[1] == '\0';
    }

    if (!IsValidPartOfMemberNameUtf8(&s)) {
      return false;
    }
  }
}

enum ClassNameType { kName, kDescriptor };
template<ClassNameType kType, char kSeparator>
static bool IsValidClassName(const char* s) {
  int arrayCount = 0;
  while (*s == '[') {
    arrayCount++;
    s++;
  }

  if (arrayCount > 255) {
    // Arrays may have no more than 255 dimensions.
    return false;
  }

  ClassNameType type = kType;
  if (type != kDescriptor && arrayCount != 0) {
    /*
     * If we're looking at an array of some sort, then it doesn't
     * matter if what is being asked for is a class name; the
     * format looks the same as a type descriptor in that case, so
     * treat it as such.
     */
    type = kDescriptor;
  }

  if (type == kDescriptor) {
    /*
     * We are looking for a descriptor. Either validate it as a
     * single-character primitive type, or continue on to check the
     * embedded class name (bracketed by "L" and ";").
     */
    switch (*(s++)) {
    case 'B':
    case 'C':
    case 'D':
    case 'F':
    case 'I':
    case 'J':
    case 'S':
    case 'Z':
      // These are all single-character descriptors for primitive types.
      return (*s == '\0');
    case 'V':
      // Non-array void is valid, but you can't have an array of void.
      return (arrayCount == 0) && (*s == '\0');
    case 'L':
      // Class name: Break out and continue below.
      break;
    default:
      // Oddball descriptor character.
      return false;
    }
  }

  /*
   * We just consumed the 'L' that introduces a class name as part
   * of a type descriptor, or we are looking for an unadorned class
   * name.
   */

  bool sepOrFirst = true;  // first character or just encountered a separator.
  for (;;) {
    uint8_t c = (uint8_t) *s;
    switch (c) {
    case '\0':
      /*
       * Premature end for a type descriptor, but valid for
       * a class name as long as we haven't encountered an
       * empty component (including the degenerate case of
       * the empty string "").
       */
      return (type == kName) && !sepOrFirst;
    case ';':
      /*
       * Invalid character for a class name, but the
       * legitimate end of a type descriptor. In the latter
       * case, make sure that this is the end of the string
       * and that it doesn't end with an empty component
       * (including the degenerate case of "L;").
       */
      return (type == kDescriptor) && !sepOrFirst && (s[1] == '\0');
    case '/':
    case '.':
      if (c != kSeparator) {
        // The wrong separator character.
        return false;
      }
      if (sepOrFirst) {
        // Separator at start or two separators in a row.
        return false;
      }
      sepOrFirst = true;
      s++;
      break;
    default:
      if (!IsValidPartOfMemberNameUtf8(&s)) {
        return false;
      }
      sepOrFirst = false;
      break;
    }
  }
}

bool IsValidBinaryClassName(const char* s) {
  return IsValidClassName<kName, '.'>(s);
}

bool IsValidJniClassName(const char* s) {
  return IsValidClassName<kName, '/'>(s);
}

bool IsValidDescriptor(const char* s) {
  return IsValidClassName<kDescriptor, '/'>(s);
}

std::string PrettyDescriptor(Primitive::Type type) {
  return PrettyDescriptor(Primitive::Descriptor(type));
}

}  // namespace libdex
