/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include <filesystem>
#include <string>

#include "ebo_base_helper.hpp"
#include <string_view>

namespace sek
{
	/** @brief Structure used to represent a platform-independent URL.
	 * @note URLs are always stored using native 8-bit `char` encoding. */
	class url
	{
	public:
		typedef char value_type;
		typedef std::string string_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		struct elements;

	public:
	private:
		string_type m_data;
		std::unique_ptr<elements> m_elements;
	};
}	 // namespace sek