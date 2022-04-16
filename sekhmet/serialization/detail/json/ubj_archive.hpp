//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include <vector>

#include "../archive_traits.hpp"
#include "../manipulators.hpp"
#include "common.hpp"

namespace sek::serialization
{
	/** @details Archive used to read UBJson data.
	 *
	 * The archive itself does not do any de-serialization, instead deserialization is done by special archive frames,
	 * which represent a Json object or array. These frames are then passed to deserialization functions
	 * of serializable types.
	 *
	 * @tparam CharType Character type used for Json. */
	template<typename CharType = char>
	class basic_ubj_input_archive : detail::json_input_archive_base<CharType>
	{
		using base_type = detail::json_input_archive_base<CharType>;

	public:
		typedef typename base_type::read_frame archive_frame;

	public:
		basic_ubj_input_archive() = delete;
		basic_ubj_input_archive(const basic_ubj_input_archive &) = delete;
		basic_ubj_input_archive &operator=(const basic_ubj_input_archive &) = delete;

		constexpr basic_ubj_input_archive(basic_ubj_input_archive &&) noexcept = default;
		constexpr basic_ubj_input_archive &operator=(basic_ubj_input_archive &&) noexcept = default;

		/** Attempts to deserialize the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return true if deserialization was successful, false otherwise. */
		template<typename T>
		constexpr bool try_read(T &&value)
		{
			return base_type::do_try_read(std::forward<T>(value));
		}
		/** Deserializes the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return Reference to this archive.
		 * @throw archive_error On deserialization errors. */
		template<typename T>
		constexpr basic_ubj_input_archive &read(T &&value)
		{
			base_type::do_read(std::forward<T>(value));
			return *this;
		}
		/** @copydoc read */
		template<typename T>
		constexpr basic_ubj_input_archive &operator>>(T &&value)
		{
			return read(std::forward<T>(value));
		}
		/** Deserializes an instance of `T` from the top-level Json entry of the archive.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<std::default_initializable T>
		constexpr T read()
		{
			T result;
			read(result);
			return result;
		}

		constexpr void swap(basic_ubj_input_archive &other) noexcept { base_type::swap(other); }
		friend constexpr void swap(basic_ubj_input_archive &a, basic_ubj_input_archive &b) noexcept { a.swap(b); }

	private:
	};

	typedef basic_ubj_input_archive<char> ubj_input_archive;

	static_assert(input_archive<ubj_input_archive::archive_frame, bool>);
	static_assert(input_archive<ubj_input_archive::archive_frame, char>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::uint8_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int8_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int16_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int32_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int64_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, float>);
	static_assert(input_archive<ubj_input_archive::archive_frame, double>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::string>);
	static_assert(container_like_archive<ubj_input_archive::archive_frame>);

	/** @details Archive used to write UBJson data. */
	class ubj_output_archive
	{
	};

	static_assert(output_archive<ubj_output_archive, bool>);
	static_assert(output_archive<ubj_output_archive, char>);
	static_assert(output_archive<ubj_output_archive, std::uint8_t>);
	static_assert(output_archive<ubj_output_archive, std::int8_t>);
	static_assert(output_archive<ubj_output_archive, std::int16_t>);
	static_assert(output_archive<ubj_output_archive, std::int32_t>);
	static_assert(output_archive<ubj_output_archive, std::int64_t>);
	static_assert(output_archive<ubj_output_archive, float>);
	static_assert(output_archive<ubj_output_archive, double>);
	static_assert(output_archive<ubj_output_archive, std::string>);
}	 // namespace sek::serialization