/*
 * JSBedded standard library for MQuickJS. A host tool built from this file and
 * upstream mquickjs_build.c writes jsb_stdlib.h (ROM tables) at configure time;
 * see engine.cmake.
 *
 * Reuses every class and object definition of upstream mqjs_stdlib.c (included
 * below, its main() renamed) and lists its own global object: the upstream set
 * minus the REPL host functions (console, print, load, setTimeout, clearTimeout).
 * Those come from firmware/core and firmware/api through engine.h at runtime,
 * as closures of the one C function declared here, jsb_host.
 */
#define main jsb_mqjs_upstream_main
#include "mqjs_stdlib.c"
#undef main

static const JSPropDef jsb_global_object[] = {
    JS_PROP_CLASS_DEF("Object", &js_object_class),
    JS_PROP_CLASS_DEF("Function", &js_function_class),
    JS_PROP_CLASS_DEF("Number", &js_number_class),
    JS_PROP_CLASS_DEF("Boolean", &js_boolean_class),
    JS_PROP_CLASS_DEF("String", &js_string_class),
    JS_PROP_CLASS_DEF("Array", &js_array_class),
    JS_PROP_CLASS_DEF("Math", &js_math_obj),
    JS_PROP_CLASS_DEF("Date", &js_date_class),
    JS_PROP_CLASS_DEF("JSON", &js_json_obj),
    JS_PROP_CLASS_DEF("RegExp", &js_regexp_class),

    JS_PROP_CLASS_DEF("Error", &js_error_class),
    JS_PROP_CLASS_DEF("EvalError", &js_eval_error_class),
    JS_PROP_CLASS_DEF("RangeError", &js_range_error_class),
    JS_PROP_CLASS_DEF("ReferenceError", &js_reference_error_class),
    JS_PROP_CLASS_DEF("SyntaxError", &js_syntax_error_class),
    JS_PROP_CLASS_DEF("TypeError", &js_type_error_class),
    JS_PROP_CLASS_DEF("URIError", &js_uri_error_class),
    JS_PROP_CLASS_DEF("InternalError", &js_internal_error_class),

    JS_PROP_CLASS_DEF("ArrayBuffer", &js_array_buffer_class),
    JS_PROP_CLASS_DEF("Uint8ClampedArray", &js_Uint8ClampedArray_class),
    JS_PROP_CLASS_DEF("Int8Array", &js_Int8Array_class),
    JS_PROP_CLASS_DEF("Uint8Array", &js_Uint8Array_class),
    JS_PROP_CLASS_DEF("Int16Array", &js_Int16Array_class),
    JS_PROP_CLASS_DEF("Uint16Array", &js_Uint16Array_class),
    JS_PROP_CLASS_DEF("Int32Array", &js_Int32Array_class),
    JS_PROP_CLASS_DEF("Uint32Array", &js_Uint32Array_class),
    JS_PROP_CLASS_DEF("Float32Array", &js_Float32Array_class),
    JS_PROP_CLASS_DEF("Float64Array", &js_Float64Array_class),

    JS_CFUNC_DEF("parseInt", 2, js_number_parseInt ),
    JS_CFUNC_DEF("parseFloat", 1, js_number_parseFloat ),
    JS_CFUNC_DEF("eval", 1, js_global_eval),
    JS_CFUNC_DEF("isNaN", 1, js_global_isNaN ),
    JS_CFUNC_DEF("isFinite", 1, js_global_isFinite ),

    JS_PROP_DOUBLE_DEF("Infinity", 1.0 / 0.0, 0 ),
    JS_PROP_DOUBLE_DEF("NaN", NAN, 0 ),
    JS_PROP_UNDEFINED_DEF("undefined", 0 ),
    /* Note: null is expanded as the global object in js_global_object[] */
    JS_PROP_NULL_DEF("globalThis", 0 ),

    JS_PROP_CLASS_DEF("performance", &js_performance_obj),
    JS_CFUNC_DEF("gc", 0, js_gc),
    JS_PROP_END,
};

/* Order matters: the adapter refers to jsb_host as JS_CFUNCTION_USER + 0. */
static const JSPropDef jsb_c_function_decl[] = {
    /* must come first if "bind" is defined */
    JS_CFUNC_SPECIAL_DEF("bound", 0, generic_params, js_function_bound ),
    JS_CFUNC_SPECIAL_DEF("jsb_host", 0, generic_params, jsb_mquickjs_host ),
    JS_PROP_END,
};

int main(int argc, char **argv)
{
    return build_atoms("js_stdlib", jsb_global_object, jsb_c_function_decl, argc, argv);
}
