#include <mruby.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/proc.h>
#include <stdlib.h>
#include <limits.h>
#define CRCEA_DEFAULT
#include "../contrib/libcrcea/src/crcea.c"
#include <mruby-aux.h>
#include <mruby-aux/scanhash.h>

#define id_crc_spec         (mrb_intern_lit(mrb, "crc:CRC@spec"))
#define id_crc_table        (mrb_intern_lit(mrb, "crc:CRC@table"))
#define id_update           (mrb_intern_lit(mrb, "update"))
#define id_initialize       (mrb_intern_lit(mrb, "initialize"))

#define ID_crc              mrb_intern_lit(mrb, "crc")
#define ID_downcase_ban     mrb_intern_lit(mrb, "downcase!")
#define ID_new              mrb_intern_lit(mrb, "new")

static void
aux_define_class_method_with_env(mrb_state *mrb, struct RClass *c, mrb_sym mid, mrb_func_t func, mrb_aspec aspec, mrb_int nenv, const mrb_value *env)
{
    struct RProc *proc = mrb_proc_new_cfunc_with_env(mrb, func, nenv, env);
    struct RClass *singleton_class = mrb_class_ptr(mrb_singleton_class(mrb, mrb_obj_value(c)));

#if MRUBY_RELEASE_NO < 10400
    mrb_define_method_raw(mrb, singleton_class, mid, proc);
#else
    mrb_method_t method;
    MRB_METHOD_FROM_PROC(method, proc);
    mrb_define_method_raw(mrb, singleton_class, mid, method);
#endif
}

static void
aux_define_class_alias(MRB, struct RClass *klass, const char *link, const char *entity)
{
    struct RClass *singleton = mrb_class_ptr(mrb_singleton_class(mrb, mrb_obj_value(klass)));
    mrb_define_alias(mrb, singleton, link, entity);
}

static int
bits_to_bytes(int bits)
{
    if (bits > 32) {
        return 8;
    } else if (bits > 16) {
        return 4;
    } else if (bits > 8) {
        return 2;
    } else {
        return 1;
    }
}

static VALUE
aux_conv_uint64(MRB, uint64_t n, int bits)
{
    int bytesize = bits_to_bytes(bits);
    int64_t m = (int64_t)n << (64 - bytesize * 8) >> (64 - bytesize * 8);
    if (m > MRB_INT_MAX || m < MRB_INT_MIN) {
        return mrb_obj_value(mrbx_str_new_as_hexdigest(mrb, m, bytesize));
    } else {
        return mrb_fixnum_value(n);
    }
}

static uint64_t
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


enum {
    CRC_MAX_BITSIZE = 64,
};

struct crcspec
{
    crcea_model model;
    crcea_context context;
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
    if (p && p->context.table) {
        mrb_free(mrb, (void *)p->context.table);
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

static void *
getrefp(MRB, VALUE obj, const mrb_data_type *type)
{
    return mrb_data_get_ptr(mrb, obj, type);
}

static void *
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

static struct crcspec *
get_specp(MRB, VALUE obj)
{
    VALUE mod = search_ivar(mrb, obj, id_crc_spec);
    if (NIL_P(mod)) { return NULL; }
    return getrefp(mrb, mod, &crcspec_type);
}

static struct crcspec *
get_spec(MRB, VALUE obj)
{
    VALUE mod = search_ivar(mrb, obj, id_crc_spec);
    if (NIL_P(mod)) {
        mrb_raisef(mrb, E_TYPE_ERROR, "not configured CRC - %S", obj);
    }
    return getref(mrb, mod, &crcspec_type);
}

static struct context *
get_contextp(MRB, VALUE obj)
{
    return getrefp(mrb, obj, &context_type);
}

static struct context *
get_context(MRB, VALUE obj)
{
    return getref(mrb, obj, &context_type);
}

static VALUE
ext_s_bitsize(MRB, VALUE crc)
{
    return mrb_fixnum_value(get_spec(mrb, crc)->model.design.bitsize);
}

static VALUE
ext_s_polynomial(MRB, VALUE crc)
{
    struct crcspec *p = get_spec(mrb, crc);
    return aux_conv_uint64(mrb, p->model.design.polynomial, p->model.design.bitsize);
}

static VALUE
ext_s_initial_crc(MRB, VALUE crc)
{
    struct crcspec *p = get_spec(mrb, crc);
    return aux_conv_uint64(mrb, p->model.initialcrc, p->model.design.bitsize);
}

static VALUE
ext_s_xor_output(MRB, VALUE crc)
{
    struct crcspec *p = get_spec(mrb, crc);
    return aux_conv_uint64(mrb, p->model.design.xoroutput, p->model.design.bitsize);
}

static VALUE
ext_s_reflect_input(MRB, VALUE crc)
{
    return get_spec(mrb, crc)->model.design.reflectin ? Qtrue : Qfalse;
}

static VALUE
ext_s_reflect_output(MRB, VALUE crc)
{
    return get_spec(mrb, crc)->model.design.reflectout ? Qtrue : Qfalse;
}

static VALUE
ext_s_append_zero(MRB, VALUE crc)
{
    return get_spec(mrb, crc)->model.design.appendzero ? Qtrue : Qfalse;
}

static void *
ext_alloc_table(struct crcspec *cc, size_t size)
{
    MRB = cc->mrb;
    void *buf = mrb_malloc(mrb, size);
    cc->context.table = buf;
    return buf;
}

static struct RData *
new_crc_module(MRB, VALUE self, int bitsize, VALUE apolynomial, VALUE initcrc, VALUE xorout, mrb_bool refin, mrb_bool refout, mrb_bool appendzero)
{
    struct RData *mod;
    struct crcspec *p;

    if (bitsize < 1 || bitsize > CRC_MAX_BITSIZE) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                "wrong ``bitsize'' (given %S, expected 1..%S)",
                mrb_fixnum_value(bitsize), mrb_fixnum_value(CRC_MAX_BITSIZE));
    }

    uint64_t polynomial = aux_to_uint64(mrb, apolynomial);
    if ((polynomial & 1) == 0) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "``polynomial'' must be an odd number (given %S)", apolynomial);
    }

    Data_Make_Struct(mrb, mrb->object_class, struct crcspec, &crcspec_type, p, mod);

    p->mrb = mrb;
    p->object = self;

    p->model.design.bitsize = bitsize;
    p->model.design.polynomial = polynomial;
    p->model.initialcrc = (NIL_P(initcrc) ? 0 : aux_to_uint64(mrb, initcrc));
    p->model.design.reflectin = (refin ? 1 : 0);
    p->model.design.reflectout = (refout ? 1 : 0);
    p->model.design.appendzero = (appendzero ? 1 : 0);
    p->model.design.xoroutput = (NIL_P(xorout) ? ~0ull : aux_to_uint64(mrb, xorout));

    p->context.design = &p->model.design;
    p->context.algorithm = CRCEA_DEFAULT_ALGORITHM;
    p->context.table = NULL;
    p->context.alloc = (crcea_alloc_f *)ext_alloc_table;
    p->context.opaque = p;

    return mod;
}

static mrb_value
redirect_crc(MRB, VALUE self)
{
    VALUE model = mrb_proc_cfunc_env_get(mrb, 0);
    mrb_int argc;
    VALUE *argv, block;

    if (mrb_get_args(mrb, "*&", &argv, &argc, &block) == 0) {
        return model;
    } else {
        return mrb_funcall_with_block(mrb, model, ID_crc, argc, argv, block);
    }
}

static void
attach_method(MRB, struct RClass *crc, VALUE model, const char *name)
{
    VALUE namev = mrb_str_new_cstr(mrb, name);
    FUNCALL(mrb, namev, ID_downcase_ban);

    aux_define_class_method_with_env(mrb, crc, mrb_obj_to_sym(mrb, namev), redirect_crc, MRB_ARGS_ANY(), 1, &model);
}

/*
 * call-seq:
 *  define_crc_module(name, bitsize, polynomial, initialcrc = 0, reflectin = true, reflectout = true, xoroutput = ~0) -> nil
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
        VALUE polynomial, initcrc, xorout;
        mrb_bool refin, refout, appendzero;
        int argc = mrb_get_args(mrb, "zio|obbob", &name, &bitsize, &polynomial, &initcrc, &refin, &refout, &xorout, &appendzero);
        if (bitsize > sizeof(crcea_int) * 8) {
            /* NOTE: 定義できないため、そのまま回れ右 */
            return Qnil;
        }
        if (argc < 4) { initcrc = Qnil; }
        if (argc < 5) { refin = 1; }
        if (argc < 6) { refout = 1; }
        if (argc < 7) { xorout = Qnil; }
        if (argc < 8) { appendzero = 1; }

        {
            struct RData *mod = new_crc_module(mrb, self, bitsize, polynomial, initcrc, xorout, refin, refout, appendzero);
            struct RClass *klass = mrb_define_class_under(mrb, selfklass, name, selfklass);
            mrb_iv_set(mrb, mrb_obj_value(klass), id_crc_spec, mrb_obj_value(mod));

            attach_method(mrb, mrb_class_ptr(self), mrb_obj_value(klass), name);

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
    VALUE polynomial, initcrc, xorout;
    mrb_bool refin, refout, appendzero;
    int argc = mrb_get_args(mrb, "io|obbob", &bitsize, &polynomial, &initcrc, &refin, &refout, &xorout, &appendzero);
    if (bitsize > sizeof(crcea_int) * 8) {
        /* NOTE: 定義できない */
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                "``bitsize`` too big (given %S, expect 1..%S)",
                mrb_fixnum_value(bitsize),
                mrb_fixnum_value(sizeof(crcea_int) * 8));
    }
    if (argc == 3 && mrb_hash_p(initcrc)) {
        mrb_value refin_v, refout_v, appendzero_v;
        MRBX_SCANHASH(mrb, initcrc, Qnil,
                MRBX_SCANHASH_ARGS("initialcrc", &initcrc, Qnil),
                MRBX_SCANHASH_ARGS("xoroutput", &xorout, Qnil),
                MRBX_SCANHASH_ARGS("reflectin", &refin_v, Qtrue),
                MRBX_SCANHASH_ARGS("reflectout", &refout_v, Qtrue),
                MRBX_SCANHASH_ARGS("appendzero", &appendzero_v, Qtrue));
        refin = mrb_bool(refin_v);
        refout = mrb_bool(refout_v);
        appendzero = mrb_bool(appendzero_v);
    } else {
        if (argc < 3) { initcrc = Qnil; }
        if (argc < 4) { refin = 1; }
        if (argc < 5) { refout = 1; }
        if (argc < 6) { xorout = Qnil; }
        if (argc < 7) { appendzero = 1; }
    }

    {
        struct RData *mod = new_crc_module(mrb, self, bitsize, polynomial, initcrc, xorout, refin, refout, appendzero);
        VALUE crcmod = FUNCALL(mrb, mrb_obj_value(mrb->class_class), ID_new, self);
        mrb_iv_set(mrb, crcmod, id_crc_spec, mrb_obj_value(mod));

        return crcmod;
    }
}

/*
 * call-seq:
 *  new(bitsize, polynomial, initialcrc = 0, reflectin = true, reflectout = true, xoroutput = ~0, algorithm = CRC::STANDARD_TABLE) -> crc generator class
 *  new(bitsize, polynomial, initialcrc: 0, reflectin: true, reflectout: true, xoroutput: ~0, algorithm: CRC::STANDARD_TABLE) -> crc generator class
 *  new(crc = 0, total_bytes = 0) -> crc generator instance
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
        cc->state = crcea_setup(&s->context, s->model.initialcrc);
    } else {
        cc->state = crcea_setup(&s->context, aux_to_uint64(mrb, initcrc));
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

    crcea_prepare_table(&cc->spec->context);
    cc->state = crcea_update(&cc->spec->context, p, p + len, cc->state);
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
        cc->state = crcea_setup(&cc->spec->context, cc->spec->model.initialcrc);
    } else {
        cc->state = crcea_setup(&cc->spec->context, aux_to_uint64(mrb, initcrc));
    }
    return self;
}

static VALUE
ext_finish(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    return aux_conv_uint64(mrb, crcea_finish(&cc->spec->context, cc->state), cc->spec->model.design.bitsize);
}

static VALUE
ext_digest(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    struct crcspec *s = cc->spec;
    int bytes = bits_to_bytes(s->model.design.bitsize);
    int off = bytes;
    VALUE str = mrb_str_buf_new(mrb, bytes);
    char *p = RSTRING_PTR(str);
    uint64_t n = crcea_finish(&s->context, cc->state);
    for (; off > 0; off -= 8, p ++) {
        *p = (n >> (off - 8)) & 0xff;
    }
    RSTR_SET_LEN(mrb_str_ptr(str), bytes);
    return str;
}

static VALUE
ext_hexdigest(MRB, VALUE self)
{
    struct context *cc = get_context(mrb, self);
    struct crcspec *s = cc->spec;
    return mrb_obj_value(mrbx_str_new_as_hexdigest(mrb,
                                                   crcea_finish(&s->context, cc->state),
                                                   bits_to_bytes(s->model.design.bitsize)));
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
    mrb_define_class_method(mrb, cCRC, "appendzero?", ext_s_append_zero, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "append_zero?", "appendzero?");
    mrb_define_class_method(mrb, cCRC, "initialcrc", ext_s_initial_crc, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "initial_crc", "initialcrc");
    mrb_define_class_method(mrb, cCRC, "xoroutput", ext_s_xor_output, MRB_ARGS_NONE());
    aux_define_class_alias(mrb, cCRC, "xor_output", "xoroutput");
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
