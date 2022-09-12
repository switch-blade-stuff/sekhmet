/*
 * Created by switchblade on 11/09/22
 */

#pragma once

#include "sekhmet/expected.hpp"

#include "object.hpp"

namespace sek::serialization
{
	template<typename>
	class basic_json_reader;

	/** @brief Structure used to parse Json data into a Json object.
	 * @tparam C Character type used by the Json object type of the reader.
	 * @tparam T Character traits of `C`.
	 * @tparam Alloc Allocator used by the Json object type of the reader. */
	template<typename C, typename T, template<typename...> typename Alloc>
	class basic_json_reader<basic_json_object<C, T, Alloc>>
	{
	public:
		typedef basic_json_object<C, T, Alloc> object_type;
		typedef T traits_type;
		typedef C char_type;

	private:
	public:
		basic_json_reader() = delete;

		/** Sets the default locale of the reader.
		 * @param loc New default locale of the reader.
		 * @return The previous locale. */
		std::locale imbue(const std::locale &loc) { return std::exchange(m_loc, loc); }

		/** @brief Uses the reader to parse a Json object.
		 * @param obj Json object to parse.
		 * @throw json_error On parsing errors. */
		void parse(object_type &obj) { return parse(obj, m_loc); }
		/** @copydoc parse */
		basic_json_reader &operator>>(object_type &obj)
		{
			parse(obj);
			return *this;
		}
		/** @copydoc parse
		 * @param loc Locale to use for parsing instead of the default one. */
		void parse(object_type &obj, const std::locale &loc);

		/** @copybrief parse
		 * @param obj Json object to parse.
		 * @return `void`, or an error code indicating a parsing error. */
		expected<void, std::error_code> parse(std::nothrow_t, object_type &obj) { return parse(obj, m_loc); }
		/** @copydoc parse
		 * @param loc Locale to use for parsing instead of the default one. */
		expected<void, std::error_code> parse(std::nothrow_t, object_type &obj, const std::locale &loc);

	private:
		std::locale m_loc;
	};

	/** @brief `basic_json_reader` alias for `json_object` Json object type. */
	using json_reader = basic_json_reader<json_object>;
}	 // namespace sek::serialization