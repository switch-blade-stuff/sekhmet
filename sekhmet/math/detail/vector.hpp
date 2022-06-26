/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "vector/type.hpp"

/* vector & mask function backend. */
#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "vector/x86/mask_double.hpp"
#include "vector/x86/mask_float.hpp"
#include "vector/x86/mask_int32.hpp"
#include "vector/x86/mask_int64.hpp"
#include "vector/x86/vector_double.hpp"
#include "vector/x86/vector_float.hpp"
#include "vector/x86/vector_int32.hpp"
#include "vector/x86/vector_int64.hpp"
#else
#warning "SMID vector operations are not implemented for this CPU"
#endif
#endif

#include "vector/generic/mask.hpp"
#include "vector/generic/vector.hpp"

/* vector & mask function definitions. */
#include "vector/func/arithmetic_impl.hpp"
#include "vector/func/bitwise_impl.hpp"
#include "vector/func/category_impl.hpp"
#include "vector/func/geometric_impl.hpp"
#include "vector/func/relational_impl.hpp"
#include "vector/func/trigonometric_impl.hpp"
#include "vector/func/utility_impl.hpp"