/*
 * Created by switchblade on 11/09/22
 */

#pragma once

#include <iostream>

#include "../../../system/char_file.hpp"
#include "object.hpp"

namespace sek
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

		/** @brief Structure used to define read callbacks. */
		struct callback_t
		{
			expected<typename traits_type::int_type, std::error_code> (*get)(void *, const std::locale &);
			expected<typename traits_type::int_type, std::error_code> (*peek)(void *, const std::locale &);
			expected<std::size_t, std::error_code> (*tell)(void *, const std::locale &);
		};

	private:
		template<typename CFrom, typename TFrom>
		[[nodiscard]] static typename traits_type::int_type convert(typename traits_type::int_type i, const std::locale &loc)
		{
			if constexpr (std::same_as<CFrom, C> && std::same_as<TFrom, T>)
				return i;
			else
			{
				auto &cvt = std::use_facet<std::codecvt<C, CFrom, std::mbstate_t>>(loc);
				const CFrom c = TFrom::to_char_type(i);

				if (cvt.always_noconv())
					return T::to_int_type(static_cast<C>(c));
				else
				{
					/* TODO: Handle encodings that require more than 4 characters. */
					C result;

					cvt.in();
				}
			}
		}

		template<typename Fc, typename Ft>
		constinit static const callback_t cfile_funcs = {
			.get = +[](void *data, const std::locale &loc) -> expected<typename traits_type::int_type, std::error_code>
			{
				auto &file = *static_cast<basic_char_file<Fc, Ft> *>(data);

				/* Read the character, make sure there is no error or EOF. */
				const auto result = file.get(std::nothrow);
				if (!result || *result == Ft::eof()) [[unlikely]]
					return result;

				return convert<Fc, Ft>(*result, loc);
			},
			.peek = +[](void *data, const std::locale &loc) -> expected<typename traits_type::int_type, std::error_code>
			{
				auto &file = *static_cast<basic_char_file<Fc, Ft> *>(data);
				file.read();
			},
			.tell = +[](void *data, const std::locale &) -> expected<std::size_t, std::error_code> {},
		};
		template<typename Sc, typename St>
		constinit static const callback_t istream_funcs = {

		};

	public:
		basic_json_reader() = delete;

		/** Initializes a Json reader using a character file and a default locale.
		 * @param file Character file containing Json data. */
		template<typename Fc, typename Ft>
		basic_json_reader(basic_char_file<Fc, Ft> &file) : basic_json_reader(cfile_funcs<Fc, Ft>, &file)
		{
		}
		/** Initializes a Json reader using a character file and the specified locale.
		 * @param file Character file containing Json data.
		 * @param loc Locale used for parsing Json data. */
		template<typename Fc, typename Ft>
		basic_json_reader(basic_char_file<Fc, Ft> &file, const std::locale &loc)
			: basic_json_reader(cfile_funcs<Fc, Ft>, &file, loc)
		{
		}

		/** Initializes a Json reader using an input stream and a default locale.
		 * @param is Input stream containing Json data. */
		template<typename Sc, typename St>
		basic_json_reader(std::basic_istream<Sc, St> &is) : basic_json_reader(istream_funcs<Sc, St>, &is)
		{
		}
		/** Initializes a Json reader using an input stream and the specified locale.
		 * @param is Input stream containing Json data.
		 * @param loc Locale used for parsing Json data. */
		template<typename Sc, typename St>
		basic_json_reader(std::basic_istream<Sc, St> &is, const std::locale &loc)
			: basic_json_reader(istream_funcs<Sc, St>, &is, loc)
		{
		}

		/** Initializes a Json reader from user-provided callbacks and data pointer and a default locale.
		 * @param callback Structure containing callbacks used for parsing.
		 * @param data Pointer to user data passed to callback functions. */
		basic_json_reader(callback_t callback, void *data) : m_funcs(callback), m_data(data) {}
		/** Initializes a Json reader from user-provided callbacks, data pointer and the specified locale.
		 * @param callback Structure containing callbacks used for parsing.
		 * @param data Pointer to user data passed to callback functions.
		 * @param loc Locale used for parsing Json data. */
		basic_json_reader(callback_t callback, void *data, const std::locale &loc)
			: m_loc(loc), m_funcs(callback), m_data(data)
		{
		}

		/** Returns the current locale of the reader. */
		[[nodiscard]] constexpr const std::locale &locale() const noexcept { return m_loc; }
		/** Sets the default locale of the reader.
		 * @param loc New default locale of the reader.
		 * @return The previous locale. */
		std::locale imbue(const std::locale &loc) { return std::exchange(m_loc, loc); }

		/** @brief Uses the reader to parse a Json object.
		 * @param obj Json object to parse.
		 * @throw archive_error On parsing errors. */
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
		callback_t m_funcs;
		void *m_data;
	};	  // namespace sek

	/** @brief `basic_json_reader` alias that uses the `json_object` object type. */
	using json_reader = basic_json_reader<json_object>;
}	 // namespace sek