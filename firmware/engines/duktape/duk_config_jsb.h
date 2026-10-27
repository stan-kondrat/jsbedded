/*
 * JSBedded overrides for Duktape's duk_config.h. engine.cmake appends
 * '#include "duk_config_jsb.h"' to the build copy of the release duk_config.h
 * (default config), so these settings win over the defaults above them.
 *
 * Only options that don't change the generated built-in data in duktape.c are
 * set here. Options listed as "present_if" in src-input/builtins.yaml (ES6, Proxy,
 * buffer objects, Symbol, Reflect, CBOR, ...) must stay at their defaults: the
 * prebuilt init data refers to their native functions. Changing those needs
 * tools/configure.py (see third_party/README.md).
 */
#ifndef DUK_CONFIG_JSB_H
#define DUK_CONFIG_JSB_H

/* --- RAM (from upstream config/examples/low_memory.yaml) --- */

/* Built-in functions become 8-byte lightfunc values instead of ~200 function
   objects of 70-80 bytes each: about 15 KB less at heap creation. */
#define DUK_USE_LIGHTFUNC_BUILTINS

/* No traceback array on every Error object. Errors still get own fileName and
   lineNumber properties (DUK_USE_AUGMENT_ERROR_CREATE, DUK_USE_PC2LINE and
   DUK_USE_FUNC_FILENAME_PROPERTY stay on), which the adapter prints. */
#undef DUK_USE_TRACEBACKS

/* String table starts at 128 slots (512 bytes) instead of 1024 and grows as
   needed. */
#undef DUK_USE_STRTAB_MINSIZE
#define DUK_USE_STRTAB_MINSIZE 128

/* No array index flag cached per string, no 256-entry literal cache. */
#undef DUK_USE_HSTRING_ARRIDX
#undef DUK_USE_LITCACHE_SIZE

/* Hash part only for objects with 64+ properties (default 8). */
#undef DUK_USE_HOBJECT_HASH_PROP_LIMIT
#define DUK_USE_HOBJECT_HASH_PROP_LIMIT 64

/* No free lists of activation and catcher records. */
#undef DUK_USE_CACHE_ACTIVATION
#undef DUK_USE_CACHE_CATCHER

/* Grow and shrink value stacks exactly, without spare. */
#undef DUK_USE_VALSTACK_GROW_SHIFT
#undef DUK_USE_VALSTACK_SHRINK_CHECK_SHIFT
#undef DUK_USE_VALSTACK_SHRINK_SLACK_SHIFT

/* --- C stack (from upstream config/examples/shallow_c_stack.yaml) --- */

#undef DUK_USE_NATIVE_CALL_RECLIMIT
#define DUK_USE_NATIVE_CALL_RECLIMIT 60
#undef DUK_USE_COMPILER_RECLIMIT
#define DUK_USE_COMPILER_RECLIMIT 50
#undef DUK_USE_REGEXP_COMPILER_RECLIMIT
#define DUK_USE_REGEXP_COMPILER_RECLIMIT 100
#undef DUK_USE_REGEXP_EXECUTOR_RECLIMIT
#define DUK_USE_REGEXP_EXECUTOR_RECLIMIT 100
#undef DUK_USE_JSON_ENC_RECLIMIT
#define DUK_USE_JSON_ENC_RECLIMIT 100
#undef DUK_USE_JSON_DEC_RECLIMIT
#define DUK_USE_JSON_DEC_RECLIMIT 100
#undef DUK_USE_MARK_AND_SWEEP_RECLIMIT
#define DUK_USE_MARK_AND_SWEEP_RECLIMIT 32

/* Calls check the C stack used since jsb_engine_eval() against
   JSB_DUKTAPE_STACK_LIMIT and throw RangeError past it (adapter_duktape.c). */
int jsb_duk_native_stack_check(void);
#undef DUK_USE_NATIVE_STACK_CHECK
#define DUK_USE_NATIVE_STACK_CHECK() jsb_duk_native_stack_check()

/* --- Fatal errors --- */

/* Fatal errors without a heap (e.g. assertions) print to the console before
   aborting, like the heap's own fatal handler. */
void jsb_duk_fatal(void *udata, const char *msg);
#undef DUK_USE_FATAL_HANDLER
#define DUK_USE_FATAL_HANDLER(udata, msg) jsb_duk_fatal((udata), (msg))

#endif
