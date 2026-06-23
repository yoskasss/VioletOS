/* VioletOS - Core type definitions */
#ifndef VIOLET_TYPES_H
#define VIOLET_TYPES_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef uint32_t size_t;
typedef int32_t  ssize_t;

typedef enum { false = 0, true = 1 } bool;

#define NULL ((void*)0)

#endif /* VIOLET_TYPES_H */
