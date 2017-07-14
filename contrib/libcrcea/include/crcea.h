/**
 * @file crcea.h
 * @brief 汎用 CRC 生成器
 * @author dearblue <dearblue@users.noreply.github.com>
 * @license Creative Commons License Zero (CC0 / Public Domain)
 */

#ifndef CRC_H__
#define CRC_H__ 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#   define CRC_BEGIN_C_DECL extern "C" {
#   define CRC_END_C_DECL   }
#else
#   define CRC_BEGIN_C_DECL
#   define CRC_END_C_DECL
#endif

enum crc_algorithms
{
    CRC_ALGORITHM_BITBYBIT          = -2,
    CRC_ALGORITHM_BITBYBIT_FAST     = -1,
    CRC_ALGORITHM_HALFBYTE_TABLE    =  0,
    CRC_ALGORITHM_STANDARD_TABLE    =  1,
    CRC_ALGORITHM_SLICING_BY_4      =  4,
    CRC_ALGORITHM_SLICING_BY_8      =  8,
    CRC_ALGORITHM_SLICING_BY_16     = 16,
};

enum crc_int_types
{
    CRC_TYPE_INT8 = 1,
    CRC_TYPE_INT16 = 2,
    CRC_TYPE_INT32 = 4,
    CRC_TYPE_INT64 = 8,
#ifdef HAVE_TYPE_UINT128_T
    CRC_TYPE_INT128 = 16,
#endif
};

typedef struct crc_t crc_t;
typedef void *(crc_alloc_f)(crc_t *cc, size_t size);

#if defined(CRC_ONLY_INT32)
typedef uint32_t crc_int;
#elif defined(CRC_ONLY_INT16)
typedef uint16_t crc_int;
#elif defined(CRC_ONLY_INT8)
typedef uint8_t crc_int;
#else /* defined(CRC_ONLY_INT64) */
typedef uint64_t crc_int;
#endif

struct crc_t
{
    int8_t inttype;    /*< enum crc_int_types */
    int8_t algorithm;  /*< enum crc_algorithms */
    int8_t bitsize;
    uint8_t reflect_input:1;
    uint8_t reflect_output:1;
    crc_int polynomial;
    crc_int xor_output;
    const void *table;
    crc_alloc_f *alloc;

    /** ... user data field ... */
};

crc_int crc_setup(crc_t *cc, crc_int crc);
crc_int crc_update(crc_t *cc, const void *src, const void *srcend, crc_int state);
crc_int crc_finish(crc_t *cc, crc_int state);

#endif /* CRC_H__ */
