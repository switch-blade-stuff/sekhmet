//
// Created by switch_blade on 4/9/2022.
//

#include <limits.h>

#include "../../define.h"

typedef enum
{
	UBJ_INVALID = 0,

	UBJ_NULL = 1,
	UBJ_NOOP = 4,
	UBJ_CHAR = 5,

	UBJ_BOOL_MASK = 2,
	UBJ_BOOL_FALSE = UBJ_BOOL_MASK,
	UBJ_BOOL_TRUE = UBJ_BOOL_MASK | 1,

	UBJ_INT_MASK = 8,
	UBJ_UINT8 = UBJ_INT_MASK | 1,
	UBJ_INT8 = UBJ_INT_MASK | 2,
	UBJ_INT16 = UBJ_INT_MASK | 3,
	UBJ_INT32 = UBJ_INT_MASK | 4,
	UBJ_INT64 = UBJ_INT_MASK | 5,

	UBJ_FLOAT_MASK = 16,
	UBJ_FLOAT32 = UBJ_FLOAT_MASK | 1,
	UBJ_FLOAT64 = UBJ_FLOAT_MASK | 2,

	UBJ_STRING_MASK = 32,
	UBJ_STRING = UBJ_STRING_MASK | 1,
	UBJ_HIGHP = UBJ_STRING_MASK | 2,

	UBJ_CONTAINER_MASK = 64,
	UBJ_ARRAY = UBJ_CONTAINER_MASK | 1,
	UBJ_OBJECT = UBJ_CONTAINER_MASK | 2,
	UBJ_TYPE_MAX,
} ubj_type_t;

#ifdef __cplusplus
extern "C"
{
	SEK_API extern const ubj_type_t sek_ubj_spec12_type_table[CHAR_MAX];
	SEK_API extern const char sek_ubj_spec12_token_table[UBJ_TYPE_MAX];
}
#else
SEK_API extern const ubj_type_t sek_ubj_spec12_type_table[CHAR_MAX];
SEK_API extern const char sek_ubj_spec12_token_table[UBJ_TYPE_MAX];
#endif
