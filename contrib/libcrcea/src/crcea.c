/**
 * @file crcea.c
 * @brief 汎用 CRC 生成器
 * @author dearblue <dearblue@users.noreply.github.com>
 * @license Creative Commons License Zero (CC0 / Public Domain)
 *
 * Preprocessor definisions before included this file:
 *
 * [CRC_NO_MALLOC]
 *      Optional, not defined by default.
 *
 * [CRC_DEFAULT_MALLOC]
 *      Optional.
 */

#include "../include/crcea.h"

#if !defined(CRC_NO_MALLOC) && !defined(CRC_DEFAULT_MALLOC)
#   define CRC_DEFAULT_MALLOC crc_default_malloc
static void *CRC_DEFAULT_MALLOC(crc_t *cc, size_t size);
#elif defined(CRC_DEFAULT_MALLOC)
void *CRC_DEFAULT_MALLOC(crc_t *cc, size_t size);
#endif

#if defined(CRC_ONLY_INT32)
#   define CRC_PREFIX      crc32
#   define CRC_TYPE        uint32_t
#   include "../include/crcea/core.h"

#   define CRC_SWITCH_BY_TYPE(C, F)     \
        F(uint32_t, crc32);             \

#elif defined(CRC_ONLY_INT16)
#   define CRC_PREFIX      crc16
#   define CRC_TYPE        uint16_t
#   include "../include/crcea/core.h"

#   define CRC_SWITCH_BY_TYPE(C, F)     \
        F(uint16_t, crc16);             \

#elif defined(CRC_ONLY_INT8)
#   define CRC_PREFIX      crc8
#   define CRC_TYPE        uint8_t
#   include "../include/crcea/core.h"

#   define CRC_SWITCH_BY_TYPE(C, F)     \
        F(uint8_t, crc8);               \

#elif defined(CRC_ONLY_INT64)
#   define CRC_PREFIX      crc64
#   define CRC_TYPE        uint64_t
#   include "../include/crcea/core.h"

#   define CRC_SWITCH_BY_TYPE(C, F)     \
        F(uint64_t, crc64);             \

#else
#   define CRC_PREFIX      crc8
#   define CRC_TYPE        uint8_t
#   include "../include/crcea/core.h"

#   define CRC_PREFIX      crc16
#   define CRC_TYPE        uint16_t
#   include "../include/crcea/core.h"

#   define CRC_PREFIX      crc32
#   define CRC_TYPE        uint32_t
#   include "../include/crcea/core.h"

#   define CRC_PREFIX      crc64
#   define CRC_TYPE        uint64_t
#   include "../include/crcea/core.h"

#   if 0 && HAVE_TYPE_UINT128_T
#      define CRC_PREFIX  crc128
#      define CRC_TYPE    uint128_t
#      include "../include/crcea/core.h"

#      define CRC_HAVE_TYPE_UINT128_T_CASE \
       case CRC_TYPE_INT128:               \
           F(uint128_t, crc128);           \
           break;                          \

#   else
#      define CRC_HAVE_TYPE_UINT128_T_CASE
#   endif

#   define CRC_SWITCH_BY_TYPE(C, F)         \
        switch (C->inttype) {               \
        case CRC_TYPE_INT8:                 \
            F(uint8_t, crc8);               \
            break;                          \
        case CRC_TYPE_INT16:                \
            F(uint16_t, crc16);             \
            break;                          \
        case CRC_TYPE_INT32:                \
            F(uint32_t, crc32);             \
            break;                          \
        case CRC_TYPE_INT64:                \
            F(uint64_t, crc64);             \
            break;                          \
        CRC_HAVE_TYPE_UINT128_T_CASE        \
        default:                            \
            break;                          \
        }                                   \

#endif /* CRC_ONLY_UINT*** */

crc_int
crc_setup(crc_t *cc, crc_int crc)
{
#define CRC_SETUP(T, P) return P ## _setup(cc, crc)

    CRC_SWITCH_BY_TYPE(cc, CRC_SETUP);

    return ~(crc_int)0;
}

crc_int
crc_update(crc_t *cc, const void *p, const void *pp, crc_int state)
{
#define CRC_UPDATE(T, P) return P ## _update(cc, p, pp, state)

    CRC_SWITCH_BY_TYPE(cc, CRC_UPDATE);

    return state;
}

crc_int
crc_finish(crc_t *cc, crc_int state)
{
#define CRC_FINISH(T, P) return P ## _finish(cc, state)

    CRC_SWITCH_BY_TYPE(cc, CRC_FINISH);

    return ~(crc_int)0;
}

#ifndef CRC_NO_MALLOC
static void *
CRC_DEFAULT_MALLOC(crc_t *cc, size_t size)
{
    return CRC_MALLOC(size);
}
#endif
