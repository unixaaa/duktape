/*
 *  Duktape built-ins
 *
 *  Size optimization note: it might seem that vararg multipurpose functions
 *  like fin(), enc(), and dec() are not very size optimal, but using a single
 *  user-visible Ecmascript function saves a lot of run-time footprint; each
 *  Function instance takes >100 bytes.  Using a shared native helper and a
 *  'magic' value won't save much if there are multiple Function instances
 *  anyway.
 */

#include "duk_internal.h"

#if defined(DUK_USE_DUKTAPE_BUILTIN)

/* Raw helper to extract internal information / statistics about a value.
 * The return values are version specific and must not expose anything
 * that would lead to security issues (e.g. exposing compiled function
 * 'data' buffer might be an issue).  Currently only counts and sizes and
 * such are given so there should not be a security impact.
 */
DUK_INTERNAL duk_ret_t duk_bi_duktape_object_info(duk_context *ctx) {
	duk_hthread *thr = (duk_hthread *) ctx;
	duk_tval *tv;
	duk_heaphdr *h;

	DUK_UNREF(thr);

	duk_push_object(ctx);
	duk_put_prop_string_uint(ctx, -1, "type", (duk_uint_t) duk_get_type(ctx, 0));

	tv = duk_get_tval(ctx, 0);
	DUK_ASSERT(tv != NULL);  /* because arg count is 1 */
	duk_put_prop_string_uint(ctx, -1, "itag", (duk_uint_t) DUK_TVAL_GET_TAG(tv));
	if (!DUK_TVAL_IS_HEAP_ALLOCATED(tv)) {
		return 1;
	}

	h = DUK_TVAL_GET_HEAPHDR(tv);
	duk_put_prop_string_pointer(ctx, -1, "hptr", (void *) h);
	DUK_ASSERT(h != NULL);

	/* refcount */
#ifdef DUK_USE_REFERENCE_COUNTING
	duk_put_prop_string_uint(ctx, -1, "refc", (duk_uint_t) DUK_HEAPHDR_GET_REFCOUNT(h));
#endif

	/* heaphdr size and additional allocation size, followed by
	 * type specific stuff (with varying value count)
	 */
	switch ((duk_small_int_t) DUK_HEAPHDR_GET_TYPE(h)) {
	case DUK_HTYPE_STRING: {
		duk_hstring *h_str = (duk_hstring *) h;
		duk_put_prop_string_uint(ctx, -1, "hbytes", (duk_uint_t) (sizeof(duk_hstring) + DUK_HSTRING_GET_BYTELEN(h_str) + 1));
		break;
	}
	case DUK_HTYPE_OBJECT: {
		duk_hobject *h_obj = (duk_hobject *) h;
		duk_small_uint_t hdr_size;
		if (DUK_HOBJECT_IS_ARRAY(h_obj)) {
			hdr_size = (duk_small_uint_t) sizeof(duk_harray);
		} else if (DUK_HOBJECT_IS_COMPFUNC(h_obj)) {
			hdr_size = (duk_small_uint_t) sizeof(duk_hcompfunc);
		} else if (DUK_HOBJECT_IS_NATFUNC(h_obj)) {
			hdr_size = (duk_small_uint_t) sizeof(duk_hnatfunc);
		} else if (DUK_HOBJECT_IS_THREAD(h_obj)) {
			hdr_size = (duk_small_uint_t) sizeof(duk_hthread);
			duk_put_prop_string_uint(ctx, -1, "tstate", ((duk_hthread *) h_obj)->state);
#if defined(DUK_USE_BUFFEROBJECT_SUPPORT)
		} else if (DUK_HOBJECT_IS_BUFOBJ(h_obj)) {
			hdr_size = (duk_small_uint_t) sizeof(duk_hbufobj);
#endif
		} else {
			hdr_size = (duk_small_uint_t) sizeof(duk_hobject);
		}

		duk_put_prop_string_uint(ctx, -1, "hbytes", hdr_size);
		duk_put_prop_string_uint(ctx, -1, "pbytes", (duk_uint_t) DUK_HOBJECT_P_ALLOC_SIZE(h_obj));
		duk_put_prop_string_uint(ctx, -1, "esize", (duk_uint_t) DUK_HOBJECT_GET_ESIZE(h_obj));
		/* Note: e_next indicates the number of gc-reachable entries
		 * in the entry part, and also indicates the index where the
		 * next new property would be inserted.  It does *not* indicate
		 * the number of non-NULL keys present in the object.  That
		 * value could be counted separately but requires a pass through
		 * the key list.
		 */
		duk_put_prop_string_uint(ctx, -1, "enext", (duk_uint_t) DUK_HOBJECT_GET_ENEXT(h_obj));
		duk_put_prop_string_uint(ctx, -1, "asize", (duk_uint_t) DUK_HOBJECT_GET_ASIZE(h_obj));
		duk_put_prop_string_uint(ctx, -1, "hsize", (duk_uint_t) DUK_HOBJECT_GET_HSIZE(h_obj));

		if (DUK_HOBJECT_IS_COMPFUNC(h_obj)) {
			duk_hbuffer *h_data = (duk_hbuffer *) DUK_HCOMPFUNC_GET_DATA(thr->heap, (duk_hcompfunc *) h_obj);
			duk_put_prop_string_uint(ctx, -1, "bcbytes", (duk_uint_t) (h_data ? DUK_HBUFFER_GET_SIZE(h_data) : 0));
		}
		break;
	}
	case DUK_HTYPE_BUFFER: {
		duk_hbuffer *h_buf = (duk_hbuffer *) h;
		if (DUK_HBUFFER_HAS_DYNAMIC(h_buf)) {
			if (DUK_HBUFFER_HAS_EXTERNAL(h_buf)) {
				duk_put_prop_string_uint(ctx, -1, "hbytes", (duk_uint_t) (sizeof(duk_hbuffer_external)));
			} else {
				/* When alloc_size == 0 the second allocation may not
				 * actually exist.
				 */
				duk_put_prop_string_uint(ctx, -1, "hbytes", (duk_uint_t) (sizeof(duk_hbuffer_dynamic)));
			}
			duk_put_prop_string_uint(ctx, -1, "dbytes", (duk_uint_t) (DUK_HBUFFER_GET_SIZE(h_buf)));
		} else {
			duk_put_prop_string_uint(ctx, -1, "hbytes", (duk_uint_t) (sizeof(duk_hbuffer_fixed) + DUK_HBUFFER_GET_SIZE(h_buf)));
		}
		break;

	}
	}
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_duktape_object_act(duk_context *ctx) {
	duk_hthread *thr = (duk_hthread *) ctx;
	duk_activation *act;
	duk_uint_fast32_t pc;
	duk_uint_fast32_t line;
	duk_int_t level;

	/* -1             = top callstack entry, callstack[callstack_top - 1]
	 * -callstack_top = bottom callstack entry, callstack[0]
	 */
	level = duk_to_int(ctx, 0);
	if (level >= 0 || -level > (duk_int_t) thr->callstack_top) {
		return 0;
	}
	DUK_ASSERT(level >= -((duk_int_t) thr->callstack_top) && level <= -1);
	act = thr->callstack + thr->callstack_top + level;

	duk_push_object(ctx);

	duk_push_tval(ctx, &act->tv_func);

	/* Relevant PC is just before current one because PC is
	 * post-incremented.  This should match what error augment
	 * code does.
	 */
	pc = duk_hthread_get_act_prev_pc(thr, act);
	duk_push_uint(ctx, (duk_uint_t) pc);

#if defined(DUK_USE_PC2LINE)
	line = duk_hobject_pc2line_query(ctx, -2, pc);
#else
	line = 0;
#endif
	duk_push_uint(ctx, (duk_uint_t) line);

	/* Providing access to e.g. act->lex_env would be dangerous: these
	 * internal structures must never be accessible to the application.
	 * Duktape relies on them having consistent data, and this consistency
	 * is only asserted for, not checked for.
	 */

	/* [ level obj func pc line ] */

	/* XXX: version specific array format instead? */
	duk_xdef_prop_stridx_wec(ctx, -4, DUK_STRIDX_LINE_NUMBER);
	duk_xdef_prop_stridx_wec(ctx, -3, DUK_STRIDX_PC);
	duk_xdef_prop_stridx_wec(ctx, -2, DUK_STRIDX_LC_FUNCTION);
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_duktape_object_gc(duk_context *ctx) {
#ifdef DUK_USE_MARK_AND_SWEEP
	duk_hthread *thr = (duk_hthread *) ctx;
	duk_small_uint_t flags;
	duk_bool_t rc;

	flags = (duk_small_uint_t) duk_get_uint(ctx, 0);
	rc = duk_heap_mark_and_sweep(thr->heap, flags);

	/* XXX: Not sure what the best return value would be in the API.
	 * Return a boolean for now.  Note that rc == 0 is success (true).
	 */
	duk_push_boolean(ctx, !rc);
	return 1;
#else
	DUK_UNREF(ctx);
	return 0;
#endif
}

#if defined(DUK_USE_FINALIZER_SUPPORT)
DUK_INTERNAL duk_ret_t duk_bi_duktape_object_fin(duk_context *ctx) {
	(void) duk_require_hobject(ctx, 0);
	if (duk_get_top(ctx) >= 2) {
		/* Set: currently a finalizer is disabled by setting it to
		 * undefined; this does not remove the property at the moment.
		 * The value could be type checked to be either a function
		 * or something else; if something else, the property could
		 * be deleted.
		 */
		duk_set_top(ctx, 2);
		(void) duk_put_prop_stridx(ctx, 0, DUK_STRIDX_INT_FINALIZER);
		return 0;
	} else {
		/* Get. */
		DUK_ASSERT(duk_get_top(ctx) == 1);
		duk_get_prop_stridx(ctx, 0, DUK_STRIDX_INT_FINALIZER);
		return 1;
	}
}
#endif  /* DUK_USE_FINALIZER_SUPPORT */

DUK_INTERNAL duk_ret_t duk_bi_duktape_object_enc(duk_context *ctx) {
	duk_hthread *thr = (duk_hthread *) ctx;
	duk_hstring *h_str;

	DUK_UNREF(thr);

	/* Vararg function: must be careful to check/require arguments.
	 * The JSON helpers accept invalid indices and treat them like
	 * non-existent optional parameters.
	 */

	h_str = duk_require_hstring(ctx, 0);
	duk_require_valid_index(ctx, 1);

	if (h_str == DUK_HTHREAD_STRING_HEX(thr)) {
		duk_set_top(ctx, 2);
		duk_hex_encode(ctx, 1);
		DUK_ASSERT_TOP(ctx, 2);
	} else if (h_str == DUK_HTHREAD_STRING_BASE64(thr)) {
		duk_set_top(ctx, 2);
		duk_base64_encode(ctx, 1);
		DUK_ASSERT_TOP(ctx, 2);
#if defined(DUK_USE_JSON_SUPPORT) && defined(DUK_USE_JX)
	} else if (h_str == DUK_HTHREAD_STRING_JX(thr)) {
		duk_bi_json_stringify_helper(ctx,
		                             1 /*idx_value*/,
		                             2 /*idx_replacer*/,
		                             3 /*idx_space*/,
		                             DUK_JSON_FLAG_EXT_CUSTOM |
		                             DUK_JSON_FLAG_ASCII_ONLY |
		                             DUK_JSON_FLAG_AVOID_KEY_QUOTES /*flags*/);
#endif
#if defined(DUK_USE_JSON_SUPPORT) && defined(DUK_USE_JC)
	} else if (h_str == DUK_HTHREAD_STRING_JC(thr)) {
		duk_bi_json_stringify_helper(ctx,
		                             1 /*idx_value*/,
		                             2 /*idx_replacer*/,
		                             3 /*idx_space*/,
		                             DUK_JSON_FLAG_EXT_COMPATIBLE |
		                             DUK_JSON_FLAG_ASCII_ONLY /*flags*/);
#endif
	} else {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}
	return 1;
}

DUK_INTERNAL duk_ret_t duk_bi_duktape_object_dec(duk_context *ctx) {
	duk_hthread *thr = (duk_hthread *) ctx;
	duk_hstring *h_str;

	DUK_UNREF(thr);

	/* Vararg function: must be careful to check/require arguments.
	 * The JSON helpers accept invalid indices and treat them like
	 * non-existent optional parameters.
	 */

	h_str = duk_require_hstring(ctx, 0);
	duk_require_valid_index(ctx, 1);

	if (h_str == DUK_HTHREAD_STRING_HEX(thr)) {
		duk_set_top(ctx, 2);
		duk_hex_decode(ctx, 1);
		DUK_ASSERT_TOP(ctx, 2);
	} else if (h_str == DUK_HTHREAD_STRING_BASE64(thr)) {
		duk_set_top(ctx, 2);
		duk_base64_decode(ctx, 1);
		DUK_ASSERT_TOP(ctx, 2);
#if defined(DUK_USE_JSON_SUPPORT) && defined(DUK_USE_JX)
	} else if (h_str == DUK_HTHREAD_STRING_JX(thr)) {
		duk_bi_json_parse_helper(ctx,
		                         1 /*idx_value*/,
		                         2 /*idx_replacer*/,
		                         DUK_JSON_FLAG_EXT_CUSTOM /*flags*/);
#endif
#if defined(DUK_USE_JSON_SUPPORT) && defined(DUK_USE_JC)
	} else if (h_str == DUK_HTHREAD_STRING_JC(thr)) {
		duk_bi_json_parse_helper(ctx,
		                         1 /*idx_value*/,
		                         2 /*idx_replacer*/,
		                         DUK_JSON_FLAG_EXT_COMPATIBLE /*flags*/);
#endif
	} else {
		DUK_DCERROR_TYPE_INVALID_ARGS(thr);
	}
	return 1;
}

/*
 *  Compact an object
 */

DUK_INTERNAL duk_ret_t duk_bi_duktape_object_compact(duk_context *ctx) {
	DUK_ASSERT_TOP(ctx, 1);
	duk_compact(ctx, 0);
	return 1;  /* return the argument object */
}

#endif  /* DUK_USE_DUKTAPE_BUILTIN */
