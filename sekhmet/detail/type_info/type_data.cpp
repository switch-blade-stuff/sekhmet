//
// Created by switch_blade on 2022-10-03.
//

#include "type_data.hpp"

#include "any.hpp"

namespace sek::detail
{
	range_type_iterator<void>::~range_type_iterator() = default;
	table_type_iterator<void>::~table_type_iterator() = default;
}	 // namespace sek::detail