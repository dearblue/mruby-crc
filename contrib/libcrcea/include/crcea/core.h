/**
 * @file crc_imps.h
 * @brief 汎用 CRC 生成器
 * @author dearblue <dearblue@users.noreply.github.com>
 * @license Creative Commons License Zero (CC0 / Public Domain)
 *
 * This is a general CRC generator.
 *
 * It's used slice-by-16 algorithm with byte-order free and byte-alignment free.
 * This is based on the Intel's slice-by-eight algorithm.
 *
 * Worst point is need more memory!
 *
 * references:
 * * https://sourceforge.net/projects/slicing-by-8/
 * * xz-utils
 *      * http://tukaani.org/xz/
 *      * xz-5.2.2/src/liblzma/check/crc32_fast.c
 *      * xz-5.2.2/src/liblzma/check/crc32_tablegen.c
 * * http://www.hackersdelight.org/hdcodetxt/crc.c.txt
 * * http://create.stephan-brumme.com/crc32/
 *
 * Preprocessor definisions before included this file:
 *
 * [CRC_PREFIX]
 *      REQUIRED.
 *
 *      This definision is undefined in last of this file.
 *
 * [CRC_TYPE]
 *      REQUIRED
 *
 *      This definision is undefined in last of this file.
 *
 * [CRC_STRIP_SIZE]
 *      optional, 1 by default.
 *
 * [CRC_VISIBILITY]
 *      optional, static by default.
 *
 * [CRC_INLINE]
 *      optional, not defined by default.
 *
 * [CRC_DEFAULT_MALLOC]
 *      optional, not defined by default.
 *
 *
 * example: mycrc.c
 *      #define CRC_PREFIX mycrc
 *      #define CRC_TYPE uint32_t
 *      #include <crc_imps.h>
 *
 *      extern uint32_t
 *      crc32(const void *ptr, const void *ptrend, uint32_t previous_crc)
 *      {
 *          static crc_t spec = {
 *              .inttype = CRC_TYPE_INT32,
 *              .algorithm = CRC_ALGORITHM_STANDARD_TABLE,
 *              .bitsize = 32,
 *              .polynomial = 0x04c11db7ul,
 *              .reflect_input = 1,
 *              .reflect_output = 1,
 *              .xor_output = ~0ul,
 *              .table = NULL,
 *              .alloc = NULL,
 *          };
 *
 *          if (!spec.table) {
 *              void *table = malloc(mycrc_tablesize(spec.algorithm));
 *              mycrc_build_table(&spec, table);
 *              spec.table = table;
 *          }
 *
 *          uint32_t state = mycrc_setup(&spec, previous_crc);
 *          state = mycrc_update_standard_table(&spec, ptr, ptrend, state);
 *          return mycrc_finish(&spec, state);
 *      }
 *
 *      extern uint32_t
 *      crc32c(const void *ptr, const void *ptrend, uint32_t previous_crc)
 *      {
 *          static crc_t spec = {
 *              .inttype = CRC_TYPE_INT32,
 *              .algorithm = CRC_ALGORITHM_SLICING_BY_4,
 *              .bitsize = 32,
 *              .polynomial = 0x1edc6f41ul,
 *              .reflect_input = 1,
 *              .reflect_output = 1,
 *              .xor_output = ~0ul,
 *              .table = NULL,
 *              .alloc = NULL,
 *          };
 *
 *          uint32_t s = mycrc_update(&spec, previous_crc);
 *          s = mycrc_update(&spec, ptr, ptrend, s);
 *          return mycrc_finish(&spec, s);
 *      }
 *
 *      extern uint32_t
 *      crc32_bzip2(const void *ptr, const void *ptrend, uint32_t previous_crc)
 *      {
 *          static crc_t spec = {
 *              .inttype = CRC_TYPE_INT32,
 *              .algorithm = CRC_ALGORITHM_BITBYBIT_FAST,
 *              .bitsize = 32,
 *              .polynomial = 0x04c11db7ul,
 *              .reflect_input = 0,
 *              .reflect_output = 0,
 *              .xor_output = ~0ul,
 *              .table = NULL,
 *              .alloc = NULL,
 *          };
 *
 *          uint32_t s = mycrc_setup(&spec, previous_crc);
 *          s = mycrc_update(&spec, ptr, ptrend, s);
 *          return mycrc_finish(&spec, s);
 *      }
 */

#if !defined(CRC_PREFIX) || !defined(CRC_TYPE)
#   error Please definision both CRC_PREFIX and CRC_TYPE macros in before include this file.
#endif

#include "../crcea.h"

#ifndef CHAR_BIT
#   include <limits.h>
#endif


#ifndef CRC_VISIBILITY
#   define CRC_VISIBILITY static
#endif

#ifndef CRC_INLINE
#   define CRC_INLINE
#endif

#ifndef CRC_NO_MALLOC
#   ifndef CRC_MALLOC
#       include <stdlib.h>
#       define CRC_MALLOC malloc
#   endif
#endif

#ifndef CRC_STRIPE_SIZE
#   define CRC_STRIPE_SIZE 1
#endif

#define CRC_TOKEN__2(PREFIX, NAME)  PREFIX ## NAME
#define CRC_TOKEN__1(PREFIX, NAME)  CRC_TOKEN__2(PREFIX, NAME)
#define CRC_TOKEN(NAME)             CRC_TOKEN__1(CRC_PREFIX, NAME)
#define CRC_BITREFLECT              CRC_TOKEN(_bitreflect)
#define CRC_SETUP                   CRC_TOKEN(_setup)
#define CRC_FINISH                  CRC_TOKEN(_finish)
#define CRC_UPDATE                  CRC_TOKEN(_update)
#define CRC_PREPARE_TABLE           CRC_TOKEN(_prepare_table)
#define CRC_TABLESIZE               CRC_TOKEN(_tablesize)
#define CRC_BUILD_TABLE             CRC_TOKEN(_build_table)
#define CRC_UPDATE_BITBYBIT         CRC_TOKEN(_update_bitbybit)
#define CRC_UPDATE_BITBYBIT_FAST    CRC_TOKEN(_update_bitbybit_fast)
#define CRC_UPDATE_HALFBYTE_TABLE   CRC_TOKEN(_update_halfbyte_table)
#define CRC_UPDATE_STANDARD_TABLE   CRC_TOKEN(_update_standard_table)
#define CRC_UPDATE_SLICING_BY_4     CRC_TOKEN(_update_slicing_by_4)
#define CRC_UPDATE_SLICING_BY_8     CRC_TOKEN(_update_slicing_by_8)
#define CRC_UPDATE_SLICING_BY_16    CRC_TOKEN(_update_slicing_by_16)

#define CRC_BITSIZE         (sizeof(CRC_TYPE) * CHAR_BIT)
#define CRC_LSH(N, OFF)     ((OFF) < CRC_BITSIZE ? (N) << (OFF) : 0)
#define CRC_RSH(N, OFF)     ((OFF) < CRC_BITSIZE ? (N) >> (OFF) : 0)
#define CRC_BITMASK(WID)    (~CRC_LSH(~(CRC_TYPE)0, WID))


CRC_BEGIN_C_DECL

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_BITREFLECT(CRC_TYPE n)
{
    if (sizeof(CRC_TYPE) <= 1) {
        n = ((n >> 4) & 0x0fu) | ( n          << 4);
        n = ((n >> 2) & 0x33u) | ((n & 0x33u) << 2);
        n = ((n >> 1) & 0x55u) | ((n & 0x55u) << 1);
    } else if (sizeof(CRC_TYPE) <= 2) {
        n = ((n >> 8) & 0x00ffu) | ( n            << 8);
        n = ((n >> 4) & 0x0f0fu) | ((n & 0x0f0fu) << 4);
        n = ((n >> 2) & 0x3333u) | ((n & 0x3333u) << 2);
        n = ((n >> 1) & 0x5555u) | ((n & 0x5555u) << 1);
    } else if (sizeof(CRC_TYPE) <= 4) {
        n = ((n >> 16) & 0x0000fffful) | ( n                 << 16);
        n = ((n >>  8) & 0x00ff00fful) | ((n & 0x00ff00fful) <<  8);
        n = ((n >>  4) & 0x0f0f0f0ful) | ((n & 0x0f0f0f0ful) <<  4);
        n = ((n >>  2) & 0x33333333ul) | ((n & 0x33333333ul) <<  2);
        n = ((n >>  1) & 0x55555555ul) | ((n & 0x55555555ul) <<  1);
    } else { /* if (sizeof(CRC_TYPE) <= 8) { */
        n = ((n >> 32) & 0x00000000ffffffffull) | ( n                          << 32);
        n = ((n >> 16) & 0x0000ffff0000ffffull) | ((n & 0x0000ffff0000ffffull) << 16);
        n = ((n >>  8) & 0x00ff00ff00ff00ffull) | ((n & 0x00ff00ff00ff00ffull) <<  8);
        n = ((n >>  4) & 0x0f0f0f0f0f0f0f0full) | ((n & 0x0f0f0f0f0f0f0f0full) <<  4);
        n = ((n >>  2) & 0x3333333333333333ull) | ((n & 0x3333333333333333ull) <<  2);
        n = ((n >>  1) & 0x5555555555555555ull) | ((n & 0x5555555555555555ull) <<  1);
    }
    return n;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_SETUP(const crc_t *cc, CRC_TYPE crc)
{
    CRC_TYPE state = (crc ^ cc->xor_output) & CRC_BITMASK(cc->bitsize);
    if (cc->reflect_input ^ cc->reflect_output) {
        state = CRC_BITREFLECT(state << (CRC_BITSIZE - cc->bitsize));
    }

    if (!cc->reflect_input) {
        state <<= (CRC_BITSIZE - cc->bitsize);
    }

    return state;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_FINISH(const crc_t *cc, CRC_TYPE state)
{
    if (!cc->reflect_input) {
        state >>= (CRC_BITSIZE - cc->bitsize);
    }

    if (cc->reflect_input ^ cc->reflect_output) {
        state = CRC_BITREFLECT(state << (CRC_BITSIZE - cc->bitsize));
    }

    state ^= cc->xor_output;

    return state & CRC_BITMASK(cc->bitsize);
}

CRC_VISIBILITY CRC_INLINE size_t
CRC_TABLESIZE(int algo)
{
    switch (algo) {
    case CRC_ALGORITHM_HALFBYTE_TABLE:
        return sizeof(CRC_TYPE[16]);
    case CRC_ALGORITHM_STANDARD_TABLE:
    case CRC_ALGORITHM_SLICING_BY_4:
    case CRC_ALGORITHM_SLICING_BY_8:
    case CRC_ALGORITHM_SLICING_BY_16:
        return sizeof(CRC_TYPE[algo][256]);
    default:
        return 0;
    }
}

CRC_VISIBILITY CRC_INLINE void
CRC_BUILD_TABLE(const crc_t *cc, void *table)
{
    int times, slice, bits;
    if (cc->algorithm < 0) {
        return;
    } else if (cc->algorithm == CRC_ALGORITHM_HALFBYTE_TABLE) {
        times = 16;
        slice = 1;
        bits = 4;
    } else {
        // CRC_ALGORITHM_STANDARD_TABLE
        // CRC_ALGORITHM_SLICING_BY_4
        // CRC_ALGORITHM_SLICING_BY_8
        // CRC_ALGORITHM_SLICING_BY_16
        times = 256;
        slice = cc->algorithm;
        bits = 8;
    }

    CRC_TYPE *t = table;
    const CRC_TYPE *tt = t;
    if (cc->reflect_input) {
        int s, b, i;
        CRC_TYPE polynomial = CRC_BITREFLECT(cc->polynomial << (CRC_BITSIZE - cc->bitsize));

        for (b = 0; b < times; b ++, t ++) {
            CRC_TYPE r = b;
            for (i = bits; i > 0; i --) {
                r = (r >> 1) ^ (polynomial & -(r & 1));
            }
            *t = r;
        }

        for (s = 1; s < slice; s ++) {
            const CRC_TYPE *q = t - times;
            for (b = 0; b < times; b ++, t ++, q ++) {
                *t = tt[*q & 0xff] ^ (*q >> 8);
            }
        }
    } else {
        int s, b, i;
        CRC_TYPE polynomial = cc->polynomial << (CRC_BITSIZE - cc->bitsize);

        for (b = 0; b < times; b ++) {
            CRC_TYPE r = (CRC_TYPE)b << (CRC_BITSIZE - bits);
            for (i = bits; i > 0; i --) {
                r = (r << 1) ^ (polynomial & -(r >> (CRC_BITSIZE - 1)));
            }
            *t ++ = r;
        }

        for (s = 1; s < slice; s ++) {
            const CRC_TYPE *q = t - times;
            for (b = 0; b < times; b ++, t ++, q ++) {
                *t = tt[*q >> (CRC_BITSIZE - 8)] ^ (*q << 8);
            }
        }
    }
}

#define CRC_SETUP_POLYNOMIAL(POLY, BS)  ((POLY) << (CRC_BITSIZE - (BS)))
#define CRC_SHIFT_INPUT(B)              ((CRC_TYPE)(uint8_t)(B) << (CRC_BITSIZE - 8))
#define CRC_POPBIT(ST, N, L)            (CRC_RSH(ST, CRC_BITSIZE - ((N) + (L))) & CRC_BITMASK(L))
#define CRC_SETUP_POLYNOMIAL_R(POLY, BS)    (CRC_BITREFLECT(POLY) >> (CRC_BITSIZE - (BS)))
#define CRC_SHIFT_INPUT_R(B)            ((CRC_TYPE)(uint8_t)(B))
#define CRC_POPBIT_R(ST, N, L)          (CRC_RSH(ST, N) & CRC_BITMASK(L))


#define CRC_UPDATE_CORE(P, PP, SLICESIZE, MAINCODE, SUBCODE)                \
    do {                                                                    \
        if (SLICESIZE > 1 || CRC_STRIPE_SIZE > 1) {                         \
            const size_t stripesize__ = CRC_STRIPE_SIZE * (SLICESIZE);      \
            const char *pps__ = P + ((PP - P) & ~(stripesize__ - 1));       \
            while (P < pps__) {                                             \
                int i__;                                                    \
                for (i__ = (CRC_STRIPE_SIZE); i__ > 0; i__ --, P += (SLICESIZE)) { \
                    MAINCODE;                                               \
                }                                                           \
            }                                                               \
        }                                                                   \
                                                                            \
        for (; P < PP; P ++) {                                              \
            if (SLICESIZE > 1) {                                            \
                SUBCODE;                                                    \
            } else {                                                        \
                MAINCODE;                                                   \
            }                                                               \
        }                                                                   \
    } while (0)                                                             \

#define CRC_UPDATE_DECL(CC, STATE, F)                                       \
    do {                                                                    \
        if ((CC)->reflect_input) {                                          \
            F(CRC_SETUP_POLYNOMIAL_R, CRC_SHIFT_INPUT_R, CRC_RSH, CRC_POPBIT_R); \
        } else {                                                            \
            F(CRC_SETUP_POLYNOMIAL, CRC_SHIFT_INPUT, CRC_LSH, CRC_POPBIT);  \
        }                                                                   \
    } while (0)                                                             \


CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_BITBYBIT(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
#define CRC_BITBYBIT_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT)     \
    CRC_TYPE poly = SETUP_POLYNOMIAL(cc->polynomial, cc->bitsize);          \
    CRC_UPDATE_CORE(p, pp, 1, {                                             \
        int i;                                                              \
        state ^= SHIFT_INPUT(*p);                                           \
        for (i = 8; i > 0; i --) {                                          \
            state = SHIFT(state, 1) ^ (poly & -POPBIT(state, 0, 1));        \
        }                                                                   \
    }, );                                                                   \

    CRC_UPDATE_DECL(cc, state, CRC_BITBYBIT_DECL);

    return state;
}

/*
 * reference:
 * * http://www.hackersdelight.org/hdcodetxt/crc.c.txt#crc32h
 */
CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_BITBYBIT_FAST(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
#define CRC_BITBYBIT_FAST_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT) \
    const CRC_TYPE g0 = SETUP_POLYNOMIAL(cc->polynomial, cc->bitsize),      \
                   g1 = SHIFT(g0, 1) ^ (g0 & -POPBIT(g0, 0, 1)),            \
                   g2 = SHIFT(g1, 1) ^ (g0 & -POPBIT(g1, 0, 1)),            \
                   g3 = SHIFT(g2, 1) ^ (g0 & -POPBIT(g2, 0, 1)),            \
                   g4 = SHIFT(g3, 1) ^ (g0 & -POPBIT(g3, 0, 1)),            \
                   g5 = SHIFT(g4, 1) ^ (g0 & -POPBIT(g4, 0, 1)),            \
                   g6 = SHIFT(g5, 1) ^ (g0 & -POPBIT(g5, 0, 1)),            \
                   g7 = SHIFT(g6, 1) ^ (g0 & -POPBIT(g6, 0, 1));            \
                                                                            \
    CRC_UPDATE_CORE(p, pp, 1, {                                             \
        state ^= SHIFT_INPUT(*p);                                           \
        state = SHIFT(state, 8) ^                                           \
                (g7 & -POPBIT(state, 0, 1)) ^                               \
                (g6 & -POPBIT(state, 1, 1)) ^                               \
                (g5 & -POPBIT(state, 2, 1)) ^                               \
                (g4 & -POPBIT(state, 3, 1)) ^                               \
                (g3 & -POPBIT(state, 4, 1)) ^                               \
                (g2 & -POPBIT(state, 5, 1)) ^                               \
                (g1 & -POPBIT(state, 6, 1)) ^                               \
                (g0 & -POPBIT(state, 7, 1));                                \
    }, );                                                                   \

    CRC_UPDATE_DECL(cc, state, CRC_BITBYBIT_FAST_DECL);

    return state;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_HALFBYTE_TABLE(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
    const CRC_TYPE *t = cc->table;

#define CRC_HALFBYTE_TABLE_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT) \
    CRC_UPDATE_CORE(p, pp, 1, {                                             \
        state ^= SHIFT_INPUT(*p);                                           \
        state = SHIFT(state, 4) ^ t[POPBIT(state, 0, 4)];                   \
        state = SHIFT(state, 4) ^ t[POPBIT(state, 0, 4)];                   \
    }, );                                                                   \

    CRC_UPDATE_DECL(cc, state, CRC_HALFBYTE_TABLE_DECL);

    return state;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_STANDARD_TABLE(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
    const CRC_TYPE *t = cc->table;

#define CRC_STANDARD_TABLE_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT) \
    CRC_UPDATE_CORE(p, pp, 1, {                                             \
        state = SHIFT(state, 8) ^ t[(uint8_t)*p ^ POPBIT(state, 0, 8)];     \
    }, );                                                                   \

    CRC_UPDATE_DECL(cc, state, CRC_STANDARD_TABLE_DECL);

    return state;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_SLICING_BY_4(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
    const CRC_TYPE (*t)[256] = cc->table;

#define CRC_SLICING_BY_4_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT) \
    CRC_UPDATE_CORE(p, pp, 4, {                                             \
        state = SHIFT(state, 32) ^                                          \
                t[3][(uint8_t)p[0] ^ POPBIT(state,  0, 8)] ^                \
                t[2][(uint8_t)p[1] ^ POPBIT(state,  8, 8)] ^                \
                t[1][(uint8_t)p[2] ^ POPBIT(state, 16, 8)] ^                \
                t[0][(uint8_t)p[3] ^ POPBIT(state, 24, 8)];                 \
    }, {                                                                    \
        state = SHIFT(state, 8) ^ t[0][(uint8_t)*p ^ POPBIT(state, 0, 8)];  \
    });                                                                     \

    CRC_UPDATE_DECL(cc, state, CRC_SLICING_BY_4_DECL);

    return state;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_SLICING_BY_8(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
    const CRC_TYPE (*t)[256] = cc->table;

#define CRC_SLICING_BY_8_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT) \
    CRC_UPDATE_CORE(p, pp, 8, {                                             \
        state = SHIFT(state, 64) ^                                          \
                t[7][(uint8_t)p[0] ^ POPBIT(state,  0, 8)] ^                \
                t[6][(uint8_t)p[1] ^ POPBIT(state,  8, 8)] ^                \
                t[5][(uint8_t)p[2] ^ POPBIT(state, 16, 8)] ^                \
                t[4][(uint8_t)p[3] ^ POPBIT(state, 24, 8)] ^                \
                t[3][(uint8_t)p[4] ^ POPBIT(state, 32, 8)] ^                \
                t[2][(uint8_t)p[5] ^ POPBIT(state, 40, 8)] ^                \
                t[1][(uint8_t)p[6] ^ POPBIT(state, 48, 8)] ^                \
                t[0][(uint8_t)p[7] ^ POPBIT(state, 56, 8)];                 \
    }, {                                                                    \
        state = SHIFT(state, 8) ^ t[0][(uint8_t)*p ^ POPBIT(state, 0, 8)];  \
    });                                                                     \

    CRC_UPDATE_DECL(cc, state, CRC_SLICING_BY_8_DECL);

    return state;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE_SLICING_BY_16(const crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
    const CRC_TYPE (*t)[256] = cc->table;

#define CRC_SLICING_BY_16_DECL(SETUP_POLYNOMIAL, SHIFT_INPUT, SHIFT, POPBIT) \
    CRC_UPDATE_CORE(p, pp, 16, {                                            \
        state = SHIFT(state, 128) ^                                         \
                t[15][(uint8_t)p[ 0] ^ POPBIT(state,   0, 8)] ^             \
                t[14][(uint8_t)p[ 1] ^ POPBIT(state,   8, 8)] ^             \
                t[13][(uint8_t)p[ 2] ^ POPBIT(state,  16, 8)] ^             \
                t[12][(uint8_t)p[ 3] ^ POPBIT(state,  24, 8)] ^             \
                t[11][(uint8_t)p[ 4] ^ POPBIT(state,  32, 8)] ^             \
                t[10][(uint8_t)p[ 5] ^ POPBIT(state,  40, 8)] ^             \
                t[ 9][(uint8_t)p[ 6] ^ POPBIT(state,  48, 8)] ^             \
                t[ 8][(uint8_t)p[ 7] ^ POPBIT(state,  56, 8)] ^             \
                t[ 7][(uint8_t)p[ 8] ^ POPBIT(state,  64, 8)] ^             \
                t[ 6][(uint8_t)p[ 9] ^ POPBIT(state,  72, 8)] ^             \
                t[ 5][(uint8_t)p[10] ^ POPBIT(state,  80, 8)] ^             \
                t[ 4][(uint8_t)p[11] ^ POPBIT(state,  88, 8)] ^             \
                t[ 3][(uint8_t)p[12] ^ POPBIT(state,  96, 8)] ^             \
                t[ 2][(uint8_t)p[13] ^ POPBIT(state, 104, 8)] ^             \
                t[ 1][(uint8_t)p[14] ^ POPBIT(state, 112, 8)] ^             \
                t[ 0][(uint8_t)p[15] ^ POPBIT(state, 120, 8)];              \
    }, {                                                                    \
        state = SHIFT(state, 8) ^ t[0][(uint8_t)*p ^ POPBIT(state, 0, 8)];  \
    });                                                                     \

    CRC_UPDATE_DECL(cc, state, CRC_SLICING_BY_16_DECL);

    return state;
}

CRC_VISIBILITY CRC_INLINE int
CRC_PREPARE_TABLE(crc_t *cc)
{
    int algo = cc->algorithm;

    if (!cc->table && algo >= CRC_ALGORITHM_HALFBYTE_TABLE) {
        crc_alloc_f *alloc = cc->alloc;
        if (!alloc) {
#ifdef CRC_DEFAULT_MALLOC
            alloc = CRC_DEFAULT_MALLOC;
#else
            return CRC_ALGORITHM_BITBYBIT_FAST;
#endif
        }

        if (alloc && algo >= CRC_ALGORITHM_HALFBYTE_TABLE) {
            void *bufp = alloc(cc, CRC_TABLESIZE(algo));
            CRC_BUILD_TABLE(cc, bufp);
            cc->table = bufp;
        }
    }

    return algo;
}

CRC_VISIBILITY CRC_INLINE CRC_TYPE
CRC_UPDATE(crc_t *cc, const char *p, const char *pp, CRC_TYPE state)
{
    int algo = CRC_PREPARE_TABLE(cc);

    switch (algo) {
    case CRC_ALGORITHM_BITBYBIT:
        return CRC_UPDATE_BITBYBIT(cc, p, pp, state);
    case CRC_ALGORITHM_HALFBYTE_TABLE:
        return CRC_UPDATE_HALFBYTE_TABLE(cc, p, pp, state);
    case CRC_ALGORITHM_STANDARD_TABLE:
        return CRC_UPDATE_STANDARD_TABLE(cc, p, pp, state);
    case CRC_ALGORITHM_SLICING_BY_4:
        return CRC_UPDATE_SLICING_BY_4(cc, p, pp, state);
    case CRC_ALGORITHM_SLICING_BY_8:
        return CRC_UPDATE_SLICING_BY_8(cc, p, pp, state);
    case CRC_ALGORITHM_SLICING_BY_16:
        return CRC_UPDATE_SLICING_BY_16(cc, p, pp, state);
    case CRC_ALGORITHM_BITBYBIT_FAST:
    default:
        return CRC_UPDATE_BITBYBIT_FAST(cc, p, pp, state);
    }
}

CRC_END_C_DECL


#undef CRC_PREFIX
#undef CRC_TYPE
#undef CRC_BUILD_TABLES
#undef CRC_BUILD_REFLECT_TABLES
#undef CRC_REFLECT_UPDATE


#undef CRC_TOKEN__2
#undef CRC_TOKEN__1
#undef CRC_TOKEN
#undef CRC_BITREFLECT
#undef CRC_SETUP
#undef CRC_FINISH
#undef CRC_UPDATE
#undef CRC_PREPARE_TABLE
#undef CRC_TABLESIZE
#undef CRC_BUILD_TABLE
#undef CRC_UPDATE_BITBYBIT
#undef CRC_UPDATE_BITBYBIT_FAST
#undef CRC_UPDATE_HALFBYTE_TABLE
#undef CRC_UPDATE_STANDARD_TABLE
#undef CRC_UPDATE_SLICING_BY_4
#undef CRC_UPDATE_SLICING_BY_8
#undef CRC_UPDATE_SLICING_BY_16
#undef CRC_BITSIZE
#undef CRC_LSH
#undef CRC_RSH
#undef CRC_BITMASK

#undef CRC_SETUP_POLYNOMIAL
#undef CRC_SHIFT_INPUT
#undef CRC_SHIFT
#undef CRC_POPBIT
#undef CRC_SETUP_POLYNOMIAL_R
#undef CRC_SHIFT_INPUT_R
#undef CRC_SHIFT_R
#undef CRC_POPBIT_R
#undef CRC_UPDATE_CORE
#undef CRC_UPDATE_DECL
#undef CRC_BITBYBIT_DECL
#undef CRC_BITBYBIT_FAST_DECL
#undef CRC_HALFBYTE_TABLE_DECL
#undef CRC_STANDARD_TABLE_DECL
#undef CRC_SLICING_BY_16_DECL
#undef CRC_SLICING_BY_4_DECL
#undef CRC_SLICING_BY_8_DECL
