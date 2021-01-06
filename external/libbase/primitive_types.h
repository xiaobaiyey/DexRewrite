#ifndef waa_BASE_PRIMITIVE_TYPES_H_
#define waa_BASE_PRIMITIVE_TYPES_H_

#include <cstdint>
#include <cstddef>

typedef uint8_t byte;
typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;


typedef int8_t s1;
typedef int16_t s2;
typedef int32_t s4;
typedef int64_t s8;

typedef size_t offset_t;
typedef void* ptr_t;


#if !defined(__ANDROID__)

/* Primitive types that match up with Java equivalents. */
typedef uint8_t jboolean; /* unsigned 8 bits */
typedef int8_t jbyte;    /* signed 8 bits */
typedef uint16_t jchar;    /* unsigned 16 bits */
typedef int16_t jshort;   /* signed 16 bits */
typedef int32_t jint;     /* signed 32 bits */
typedef int64_t jlong;    /* signed 64 bits */
typedef float jfloat;   /* 32-bit IEEE 754 */
typedef double jdouble;  /* 64-bit IEEE 754 */

typedef union jvalue {
    jboolean z;
    jbyte b;
    jchar c;
    jshort s;
    jint i;
    jlong j;
    jfloat f;
    jdouble d;
    char *l;
} jvalue;

#endif

#endif  // waa_BASE_PRIMITIVE_TYPES_H_
