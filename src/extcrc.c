#include <mruby.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/data.h>
#include <stdlib.h>
#include <limits.h>
#include "crc.h"
#include "mrbx_kwargs.h"

#define Qnil mrb_nil_value()
#define NIL_P(obj) mrb_nil_p(obj)
#define Qtrue mrb_true_value()
#define Qfalse mrb_false_value()
#define MRB mrb_state *mrb
#define VALUE mrb_value

#define FUNCALL(mrb, recv, mid, ...) ({                                     \
        VALUE args__[] = { __VA_ARGS__ };                                   \
        mrb_funcall_argv((mrb), (recv), mrb_intern_cstr((mrb), (mid)),      \
                sizeof(args__) / sizeof(args__[0]), args__);                \
    })                                                                      \

#define id_crc_spec         (mrb_intern_lit(mrb, "crc:CRC@spec"))
#define id_crc_table        (mrb_intern_lit(mrb, "crc:CRC@table"))
#define id_update           (mrb_intern_lit(mrb, "update"))
#define id_initialize       (mrb_intern_lit(mrb, "initialize"))
#define id_bitbybit         (mrb_intern_lit(mrb, "bitbybit"))
#define id_bitbybit_fast    (mrb_intern_lit(mrb, "bitbybit_fast"))
#define id_halfbyte_table   (mrb_intern_lit(mrb, "halfbyte_table"))
#define id_standard_table   (mrb_intern_lit(mrb, "standard_table"))
#define id_slicing_by_4     (mrb_intern_lit(mrb, "slicing_by_4"))
#define id_slicing_by_8     (mrb_intern_lit(mrb, "slicing_by_8"))
#define id_slicing_by_16    (mrb_intern_lit(mrb, "slicing_by_16"))

#if     defined(CRC_DEFAULT_BITBYBIT)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_BITBYBIT
#elif   defined(CRC_DEFAULT_BITBYBIT_FAST)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_BITBYBIT_FAST
#elif   defined(CRC_DEFAULT_HALFBYTE_TABLE)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_HALFBYTE_TABLE
#elif   defined(CRC_DEFAULT_STANDARD_TABLE)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_STANDARD_TABLE
#elif   defined(CRC_DEFAULT_SLICING_BY_4)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_SLICING_BY_4
#elif   defined(CRC_DEFAULT_SLICING_BY_8)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_SLICING_BY_8
#elif   defined(CRC_DEFAULT_SLICING_BY_16)
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_SLICING_BY_16
#else
#   define CRC_DEFAULT_ALGORITHM CRC_ALGORITHM_STANDARD_TABLE
#endif

static inline void
aux_define_class_alias(MRB, struct RClass *klass, const char *link, const char *entity)
{
    struct RClass *singleton = mrb_class_ptr(mrb_singleton_class(mrb, mrb_obj_value(klass)));
    mrb_define_alias(mrb, singleton, link, entity);
}

static inline VALUE
aux_conv_hexdigest(MRB, uint64_t n, int bytesize)
{
    int off = bytesize * 8;
    char str[bytesize * 2];
    char *p = str;
    for (; off > 0; off -= 4, p ++) {
        uint8_t ch = (n >> (off - 4)) & 0x0f;
        if (ch < 10) {
            *p = '0' + ch;
        } else {
            *p = 'a' - 10 + ch;
        }
    }

    return mrb_str_new(mrb, str, bytesize * 2);
}

static inline VALUE
aux_conv_uint64(MRB, uint64_t n, int bytesize)
{
    int64_t m = (int64_t)n << (64 - bytesize * 8) >> (64 - bytesize * 8);
    if (m > MRB_INT_MAX || m < MRB_INT_MIN) {
        return aux_conv_hexdigest(mrb, m, bytesize);
    } else {
        return mrb_fixnum_value(n);
    }
}

static inline uint64_t
aux_to_uint64(MRB, VALUE n)
{
    if (mrb_float_p(n)) {
        return mrb_float(n);
    } else if (mrb_string_p(n)) {
        return strtoull(RSTRING_PTR(n), NULL, 16);
    } else {
        return mrb_int(mrb, n);
    }
}

static inline int
aux_to_algorithm(MRB, VALUE algo)
{
    if (NIL_P(algo)) {
        return CRC_ALGORITHM_STANDARD_TABLE;
    } else if (mrb_symbol_p(algo)) {
        mrb_sym id = mrb_symbol(algo);

        if (id == id_standard_table) {
            return CRC_ALGORITHM_STANDARD_TABLE;
        } else if (id == id_slicing_by_4) {
            return CRC_ALGORITHM_SLICING_BY_4;
        } else if (id == id_slicing_by_8) {
            return CRC_ALGORITHM_SLICING_BY_8;
        } else if (id == id_slicing_by_16) {
            return CRC_ALGORITHM_SLICING_BY_16;
        } else if (id == id_halfbyte_table) {
            return CRC_ALGORITHM_HALFBYTE_TABLE;
        } else if (id == id_bitbybit_fast) {
            return CRC_ALGORITHM_BITBYBIT_FAST;
        } else if (id == id_bitbybit) {
            return CRC_ALGORITHM_BITBYBIT;
        }
    }

    mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong algorithm (%S)", algo);
}

static inline int
aux_bitsize_to_type(int bitsize)
{
    if (bitsize > 32) {
        return CRC_TYPE_INT64;
    } else if (bitsize > 16) {
        return CRC_TYPE_INT32;
    } else if (bitsize > 8) {
        return CRC_TYPE_INT16;
    } else {
        return CRC_TYPE_INT8;
    }
}


enum {
    CRC_MAX_BITSIZE = 64,
};

struct crcspec
{
    crc_t basic;
    uint64_t initial_crc;
    mrb_state *mrb;
    VALUE object;
};

struct context
{
    struct crcspec *spec;
    uint64_t state;
    uint64_t total_input;
};

static void
spec_free(MRB, void *ptr)
{
    struct crcspec *p = ptr;
    if (p && p->basic.table) {
        mrb_free(mrb, (void *)p->basic.table);
    }
    mrb_free(mrb, p);
}

static const mrb_data_type crcspec_type = {
    .struct_name = "mruby-crc.spec",
    .dfree = spec_free,
};

static const mrb_data_type context_type = {
    .struct_name = "mruby-crc.context",
    .dfree = mrb_free,
};

static inline void *
getrefp(MRB, VALUE obj, const mrb_data_type *type)
{
    void *p;
    Data_Get_Struct(mrb, obj, type, p);
    return p;
}

static inline void *
getref(MRB, VALUE obj, const mrb_data_type *type)
{
    void *p = getrefp(mrb, obj, type);
    if (!p) {
        mrb_raisef(mrb, E_TYPE_ERROR, "invalid initialized pointer - %S", obj);
    }
    return p;
}

static VALUE
search_ivar(MRB, VALUE obj, mrb_sym id)
{
    VALUE mod;
    for (;;) {
        mod = mrb_iv_get(mrb, obj, id);
        if (!NIL_P(mod)) {
            return mod;
        }
        struct RClass *c = RCLASS_SUPER(obj);
        if (!c) {
            return Qnil;
        }
        obj = mrb_obj_value(c);
    } 
}

static inline struct crcspec *
get_specp(MRB, VALUE obj)
{
    VALUE mod = search_ivar(mrb, obj, id_crc_spec);
    if (NIL_P(mod)) { return NULL; }
    return getrefp(mrb, mod, &crcspec_type);
}

static inline struct crcspec *
get_spec(MRB, VALUE obj)
{
    VALUE mod = search_ivar(mrb, obj, id_crc_spec);
    if (NIL_P(mod)) {
        mrb_raisef(mrb, E_TYPE_ERROR, "not configured CRC - %S", obj);
    }
    return getref(mrb, mod, &crcspec_type);
}

static inline struct context *
get_contextp(MRB, VALUE obj)
{
    return getrefp(mrb, obj, &context_type);
}

static inline struct context *
get_context(MRB, VALUE obj)
{
    return getref(mrb, obj, &context_type);
}

static VALUE
ext_s_bitsize(MRB, VALUE crc)
{
    return mrb_fixnum_value(get_spec(mrb, crc)->basic.bitsize);
}

static VALUE
ext_s_polynomial(MRB, VALUE crc)
{
    struct crcspec *p = get_spec(mrb, crc);
    return aux_conv_uint64(mrb, p->basic.polynomial, p->basic.inttype);
}

static VALUE
ext_s_initial_crc(MRB, VALUE crc)
{
    struct crcspec *p = get_spec(mrb, crc);
    return aux_conv_uint64(mrb, p->initial_crc, p->basic.inttype);
}

static VALUE
ext_s_xor_output(MRB, VALUE crc)
{
    struct crcspec *p = get_spec(mrb, crc);
    return aux_conv_uint64(mrb, p->basic.xor_output, p->basic.inttype);
}

static VALUE
ext_s_reflect_input(MRB, VALUE crc)
{
    return get_spec(mrb, crc)->basic.reflect_input ? Qtrue : Qfalse;
}

static VALUE
ext_s_reflect_output(MRB, VALUE crc)
{
    return get_spec(mrb, crc)->basic.reflect_output ? Qtrue : Qfalse;
}

static VALUE
ext_s_algorithm(MRB, VALUE crc)
{
    switch (get_spec(mrb, crc)->basic.algorithm) {
    case CRC_ALGORITHM_BITBYBIT:        return mrb_symbol_value(id_bitbybit);
    case CRC_ALGORITHM_BITBYBIT_FAST:   return mrb_symbol_value(id_bitbybit_fast);
    case CRC_ALGORITHM_HALFBYTE_TABLE:  return mrb_symbol_value(id_halfbyte_table);
    case CRC_ALGORITHM_STANDARD_TABLE:  return mrb_symbol_value(id_standard_table);
    case CRC_ALGORITHM_SLICING_BY_4:    return mrb_symbol_value(id_slicing_by_4);
    case CRC_ALGORITHM_SLICING_BY_8:    return mrb_symbol_value(id_slicing_by_8);
    case CRC_ALGORITHM_SLICING_BY_16:   return mrb_symbol_value(id_slicing_by_16);
    default: mrb_bug(mrb, "unknown algorithm (%d)", get_spec(mrb, crc)->basic.algorithm);
    }
}

static void *
ext_alloc_table(struct crcspec *cc, size_t size)
{
    MRB = cc->mrb;
    void *buf = mrb_malloc(mrb, size);
    cc->basic.table = buf;
    return buf;
}

static struct RData *
new_crc_module(MRB, VALUE self, int bitsize, VALUE polynomial, VALUE initcrc, VALUE xorout, VALUE algo, mrb_bool refin, mrb_bool refout)
{
    struct RData *mod;
    struct crcspec *p;

    if (bitsize < 1 || bitsize > CRC_MAX_BITSIZE) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "wrong ``bitsize'' (given %d, expected %d..%d)", bitsize, 1, CRC_MAX_BITSIZE);
    }

    Data_Make_Struct(mrb, mrb->object_class, struct crcspec, &crcspec_type, p, mod);

    p->mrb = mrb;
    p->object = self;
    p->basic.inttype = aux_bitsize_to_type(bitsize);

    p->basic.bitsize = bitsize;
    p->basic.polynomial = aux_to_uint64(mrb, polynomial);
    p->initial_crc = (NIL_P(initcrc) ? 0 : aux_to_uint64(mrb, initcrc));
    p->basic.reflect_input = (refin ? 1 : 0);
    p->basic.reflect_output = (refout ? 1 : 0);
    p->basic.xor_output = (NIL_P(xorout) ? ~0ull : aux_to_uint64(mrb, xorout));

    p->basic.algorithm = (NIL_P(algo) ? CRC_DEFAULT_ALGORITHM : aux_to_algorithm(mrb, algo));
    p->basic.table = NULL;
    p->basic.alloc = (crc_alloc_f *)ext_alloc_table;

    return mod;
}

/*
 * call-seq:
 *  define_crc_module(name, bitsize, polynomial, initial_crc = 0, reflect_input = true, reflect_output = true, xor_output = ~0, algorithm = CRC::STANDARD_TABLE) -> nil
 */
static VALUE
ext_s_define(MRB, VALUE self)
{
    // self が ::CRC であることを確認

    struct RClass *selfklass = mrb_class_ptr(self);
    if (selfklass != mrb_class_get(mrb, "CRC")) {
        mrb_raisef(mrb, E_TYPE_ERROR,
                "wrong call from out of CRC class - %S", self);
    }

    {
        char *name;
        mrb_int bitsize;
        VALUE polynomial, initcrc, xorout, algo;
        mrb_bool refin, refout;
        int argc = mrb_get_args(mrb, "zio|obboo", &name, &bitsize, &polynomial, &initcrc, &refin, &refout, &xorout, &algo);
        if (bitsize > sizeof(crc_int) * 8) {
            /* NOTE: 定義できないため、そのまま回れ右 */
            return Qnil;
        }
        if (argc < 4) { initcrc = Qnil; }
        if (argc < 5) { refin = 1; }
        if (argc < 6) { refout = 1; }
        if (argc < 7) { xorout = Qnil; }
        if (argc < 8) { algo = Qnil; }

        {
            struct RData *mod = new_crc_module(mrb, self, bitsize, polynomial, initcrc, xorout, algo, refin, refout);
            struct RClass *klass = mrb_define_class_under(mrb, selfklass, name, selfklass);
            mrb_iv_set(mrb, mrb_obj_value(klass), id_crc_spec, mrb_obj_value(mod));

            return Qnil;
        }
    }
}

static VALUE
ext_s_new_context(MRB, VALUE self)
{
    // CRC インスタンスを生成
    VALUE crc = mrb_obj_value(Data_Wrap_Struct(mrb, mrb_class_ptr(self), &context_type, NULL));

    // #initialize を呼び出す (初期化はそちらで行う)
    mrb_int argc;
    VALUE *argv, block;
    mrb_get_args(mrb, "*&", &argv, &argc, &block);
    mrb_funcall_with_block(mrb, crc, id_initialize, argc, argv, block);

    return crc;
}

static VALUE
ext_s_new_define(MRB, VALUE self)
{
    mrb_int bitsize;
    VALUE polynomial, initcrc, xorout, algo;
    mrb_bool refin, refout;
    int argc = mrb_get_args(mrb, "io|obboo", &bitsize, &polynomial, &initcrc, &refin, &refout, &xorout, &algo);
    if (bitsize > sizeof(crc_int) * 8) {
        /* NOTE: 定義できない */
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                "``bitsize`` too big (given %S, expect 1..%S)",
                mrb_fixnum_value(bitsize),
                mrb_fixnum_value(sizeof(crc_int) * 8));
    }
    if (argc == 3 && mrb_hash_p(initcrc)) {
        mrb_value refin_v, refout_v;
        MRBX_SCANHASH(mrb, initcrc, Qnil,
                MRBX_SCANHASH_ARGS("initialcrc", &initcrc, Qnil),
                MRBX_SCANHASH_ARGS("xoroutput", &xorout, Qnil),
                MRBX_SCANHASH_ARGS("reflectin", &refin_v, Qtrue),
                MRBX_SCANHASH_ARGS("reflectout", &refout_v, Qtrue),
                MRBX_SCANHASH_ARGS("algorithm", &algo, Qnil));
        refin = mrb_bool(refin_v);
        refout = mrb_bool(refout_v);
    } else {
        if (argc < 3) { initcrc = Qnil; }
        if (argc < 4) { refin = 1; }
        if (argc < 5) { refout = 1; }
        if (argc < 6) { xorout = Qnil; }
        if (argc < 7) { algo = Qnil; }
    }

    {
        struct RData *mod = new_crc_module(mrb, self, bitsize, polynomial, initcrc, xorout, algo, refin, refout);
        VALUE crcmod = FUNCALL(mrb, mrb_obj_value(mrb->class_class), "new", self);
        mrb_iv_set(mrb, crcmod, id_crc_spec, mrb_obj_value(mod));

        return crcmod;
    }
}

/*
 * call-seq:
 *  new(bitsize, polynomial, initial_crc = 0, reflect_input = true, reflect_output = true, xor_output = ~0, algorithm = CRC::STANDARD_TABLE) -> crc generator class
 *  new(bitsize, polynomial, initial_crc: 0, reflect_input: true, reflect_output: true, xor_output: ~0, algorithm: CRC::STANDARD_TABLE) -> crc generator class
 *  new(continuous_crc = 0, total_bytes = 0) -> crc generator instance
 */
static VALUE
ext_s_new(MRB, VALUE self)
{
    struct crcspec *s = get_specp(mrb, self);
    if (s) {
        return ext_s_new_context(mrb, self);
    } else {
        return ext_s_new_define(mrb, self);
    }
}

static VALUE
ext_s_is_configured(MRB, VALUE self)
{
    if (get_specp(mrb, self)) {
        return Qtrue;
    } else {
        return Qfalse;
    }
}

static void
ext_initialize_args(MRB, VALUE *initcrc, VALUE *totalin)
{
    *initcrc = *totalin = Qnil;
    mrb_get_args(mrb, "|oo", initcrc, totalin);
}

static VALUE
ext_initialize(MRB, VALUE self)
{
    VALUE initcrc, totalin;
    ext_initialize_args(mrb, &initcrc, &totalin);

    struct context *cc = get_contextp(mrb, self);
    if (cc) {
        mrb_raisef(mrb, E_TYPE_ERROR, "initialized already - %S", self);
    }

    struct RClass *mod = mrb_obj_class(mrb, self);
    struct crcspec *s = get_spec(mrb, mrb_obj_value(mod));

    DATA_PTR(self) = cc = mrb_malloc(mrb, sizeof(struct context));
    cc->spec = s;

    cc->total_input = (NIL_P(totalin) ? 0 : aux_to_uint64(mrb, totalin));
    if (NIL_P(initcrc)) {
        cc->state = crc_setup(&s->basic, s->initial_crc);
    } else {
        cc->state = crc_setup(&s->basic, aux_to_uint64(mrb, initcrc));
    }

    return self;
}

//mrb_define_method(mrb, cCRC, "initialize_copy", ext_initialize_copy, MRB_ARGS_REQ(1));
//mrb_define_method(mrb, cCRC, "==", ext_op_eq, MRB_ARGS_REQ(1));
//mrb_define_method(mrb, cCRC, "eql?", ext_eql, MRB_ARGS_REQ(1));

static VALUE
ext_update(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    const char *p;
    mrb_int len;
    mrb_get_args(mrb, "s", &p, &len);

    cc->state = crc_update(&cc->spec->basic, p, p + len, cc->state);
    cc->total_input += len;

    return self;
}

static VALUE
ext_reset(MRB, VALUE self)
{
    VALUE initcrc = Qnil, totalin = Qnil;
    struct context *cc = get_context(mrb, self);
    mrb_get_args(mrb, "|oo", &initcrc, &totalin);
    cc->total_input = (NIL_P(totalin) ? 0 : aux_to_uint64(mrb, totalin));
    if (NIL_P(initcrc)) {
        cc->state = crc_setup(&cc->spec->basic, cc->spec->initial_crc);
    } else {
        cc->state = crc_setup(&cc->spec->basic, aux_to_uint64(mrb, initcrc));
    }
    return self;
}

static VALUE
ext_finish(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    return aux_conv_uint64(mrb, crc_finish(&cc->spec->basic, cc->state), cc->spec->basic.inttype);
}

static VALUE
ext_digest(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    struct crcspec *s = cc->spec;
    int off = s->basic.inttype * 8;
    VALUE str = mrb_str_buf_new(mrb, s->basic.inttype);
    char *p = RSTRING_PTR(str);
    uint64_t n = crc_finish(&s->basic, cc->state);
    for (; off > 0; off -= 8, p ++) {
        *p = (n >> (off - 8)) & 0xff;
    }
    RSTR_SET_LEN(mrb_str_ptr(str), s->basic.inttype);
    return str;
}

static VALUE
ext_hexdigest(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    struct crcspec *s = cc->spec;
    return aux_conv_hexdigest(mrb,
            crc_finish(&s->basic, cc->state), s->basic.inttype);
}

static VALUE
ext_size(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    return mrb_fixnum_value(cc->total_input);
}

void
mrb_mruby_crc_gem_init(MRB)
{
    struct RClass *cCRC = mrb_define_class(mrb, "CRC", mrb->object_class);
    mrb_define_const(mrb, cCRC, "BITBYBIT", mrb_symbol_value(id_bitbybit));
    mrb_define_const(mrb, cCRC, "BITBYBIT_FAST", mrb_symbol_value(id_bitbybit_fast));
    mrb_define_const(mrb, cCRC, "HALFBYTE_TABLE", mrb_symbol_value(id_halfbyte_table));
    mrb_define_const(mrb, cCRC, "STANDARD_TABLE", mrb_symbol_value(id_standard_table));
    mrb_define_const(mrb, cCRC, "SLICING_BY_4", mrb_symbol_value(id_slicing_by_4));
    mrb_define_const(mrb, cCRC, "SLICING_BY_8", mrb_symbol_value(id_slicing_by_8));
    mrb_define_const(mrb, cCRC, "SLICING_BY_16", mrb_symbol_value(id_slicing_by_16));

    mrb_define_class_method(mrb, cCRC, "define_crc_module", ext_s_define, MRB_ARGS_ANY());

    mrb_define_class_method(mrb, cCRC, "new", ext_s_new, MRB_ARGS_ANY());
    mrb_define_class_method(mrb, cCRC, "configured?", ext_s_is_configured, MRB_ARGS_ANY());
    //mrb_define_class_method(mrb, cCRC, "eql?", ext_s_eql, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, cCRC, "polynomial", ext_s_polynomial, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, cCRC, "bitsize", ext_s_bitsize, MRB_ARGS_NONE());
    //mrb_define_class_method(mrb, cCRC, "bitmask", ext_s_bitmask, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, cCRC, "reflectin?", ext_s_reflect_input, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "reflect_input?", "reflectin?");
    mrb_define_class_method(mrb, cCRC, "reflectout?", ext_s_reflect_output, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "reflect_output?", "reflectout?");
    mrb_define_class_method(mrb, cCRC, "initialcrc", ext_s_initial_crc, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "initial_crc", "initialcrc");
    mrb_define_class_method(mrb, cCRC, "xoroutput", ext_s_xor_output, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "xor_output", "xoroutput");
    mrb_define_class_method(mrb, cCRC, "algorithm", ext_s_algorithm, MRB_ARGS_NONE());
    //mrb_define_class_method(mrb, cCRC, "table", ext_s_table, MRB_ARGS_NONE());
#if 0
    mrb_define_class_method(mrb, cCRC, "combine", ext_s_combine, MRB_ARGS_REQ(3));
#endif

    //mrb_define_alias(mrb, mrb_singleton_class(mrb), cCRC, "[]", "new");
    mrb_define_method(mrb, cCRC, "initialize", ext_initialize, MRB_ARGS_ANY());
    //mrb_define_method(mrb, cCRC, "initialize_copy", ext_initialize_copy, MRB_ARGS_REQ(1));
    //mrb_define_method(mrb, cCRC, "==", ext_op_eq, MRB_ARGS_REQ(1));
    //mrb_define_method(mrb, cCRC, "eql?", ext_eql, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, cCRC, "update", ext_update, MRB_ARGS_REQ(1));
    mrb_define_alias(mrb, cCRC, "<<", "update");
    mrb_define_method(mrb, cCRC, "reset", ext_reset, MRB_ARGS_ARG(0, 2));
    mrb_define_method(mrb, cCRC, "finish", ext_finish, MRB_ARGS_NONE());
    mrb_define_method(mrb, cCRC, "digest", ext_digest, MRB_ARGS_NONE());
    mrb_define_method(mrb, cCRC, "hexdigest", ext_hexdigest, MRB_ARGS_NONE());
    mrb_define_method(mrb, cCRC, "size", ext_size, MRB_ARGS_NONE());
    //mrb_define_method(mrb, cCRC, "combine", ext_combine, MRB_ARGS_REQ(1));
    //mrb_define_alias(mrb, cCRC, "+", "combine");
}

void
mrb_mruby_crc_gem_final(MRB)
{
}
