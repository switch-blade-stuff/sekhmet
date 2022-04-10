//
// Created by switch_blade on 4/9/2022.
//

#include "ubj_spec12_types.h"

const ubj_type_t sek_ubj_spec12_type_table[CHAR_MAX] = {
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

const char sek_ubj_spec12_token_table[UBJ_TYPE_MAX] = {
	[UBJ_NULL] = 'Z',
	[UBJ_NOOP] = 'N',
	[UBJ_CHAR] = 'C',
	[UBJ_BOOL_FALSE] = 'F',
	[UBJ_BOOL_TRUE] = 'T',
	[UBJ_UINT8] = 'U',
	[UBJ_INT8] = 'i',
	[UBJ_INT16] = 'I',
	[UBJ_INT32] = 'l',
	[UBJ_INT64] = 'L',
	[UBJ_FLOAT32] = 'd',
	[UBJ_FLOAT64] = 'D',
	[UBJ_STRING] = 'S',
	[UBJ_HIGHP] = 'H',
	[UBJ_OBJECT] = '{',
	[UBJ_ARRAY] = '[',
};