//
// Created by switch_blade on 4/9/2022.
//

#include "ubj_spec12_types.h"

ubj_type_t ubj_spec12_type_table[CHAR_MAX] = {
	UBJ_INVALID,
	['Z'] = UBJ_NULL,
	['N'] = UBJ_NOOP,
	['C'] = UBJ_CHAR,

	['F'] = UBJ_BOOL_FALSE,
	['T'] = UBJ_BOOL_TRUE,

	['U'] = UBJ_UINT8,
	['i'] = UBJ_INT8,
	['I'] = UBJ_INT16,
	['l'] = UBJ_INT32,
	['L'] = UBJ_INT64,

	['d'] = UBJ_FLOAT32,
	['D'] = UBJ_FLOAT64,

	['S'] = UBJ_STRING,
	['H'] = UBJ_HIGHP,

	['{'] = UBJ_OBJECT,
	['['] = UBJ_ARRAY,
};