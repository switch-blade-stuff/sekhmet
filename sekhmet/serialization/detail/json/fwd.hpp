/*
 * Created by switchblade on 22/08/22
 */

#pragma once

namespace sek::serialization
{
	enum class json_type : std::uint16_t;

	template<typename, typename, template<typename...> typename>
	class basic_json_value;
}	 // namespace sek::serialization