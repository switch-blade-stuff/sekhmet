/*
 * Created by switchblade on 12/06/22
 */

#pragma once

#include "archive_reader.hpp"
#include "manipulators.hpp"
#include "sekhmet/detail/bswap.hpp"
#include "util.hpp"

namespace sek::serialization::binary
{
	typedef int config_flags;
	constexpr config_flags no_flags = 0;

	/** Data is read & written in big-endian mode. */
	constexpr config_flags big_endian = 1;
	/** Data is read & written in little-endian mode. */
	constexpr config_flags little_endian = 2;

	namespace detail
	{
		using namespace serialization::detail;

		template<config_flags C, typename T>
		constexpr T fix_endianness(T value) noexcept
			requires(C == big_endian)
		{
			if constexpr (sizeof(T) == sizeof(std::uint16_t))
				return std::bit_cast<T>(BSWAP_BE_16(std::bit_cast<std::uint16_t>(value)));
			else if constexpr (sizeof(T) == sizeof(std::uint32_t))
				return std::bit_cast<T>(BSWAP_BE_32(std::bit_cast<std::uint32_t>(value)));
			else if constexpr (sizeof(T) == sizeof(std::uint64_t))
				return std::bit_cast<T>(BSWAP_BE_64(std::bit_cast<std::uint64_t>(value)));
			else
				return value;
		}
		template<config_flags C, typename T>
		constexpr T fix_endianness(T value) noexcept
			requires(C == little_endian)
		{
			if constexpr (sizeof(T) == sizeof(std::uint16_t))
				return std::bit_cast<T>(BSWAP_LE_16(std::bit_cast<std::uint16_t>(value)));
			else if constexpr (sizeof(T) == sizeof(std::uint32_t))
				return std::bit_cast<T>(BSWAP_LE_32(std::bit_cast<std::uint32_t>(value)));
			else if constexpr (sizeof(T) == sizeof(std::uint64_t))
				return std::bit_cast<T>(BSWAP_LE_64(std::bit_cast<std::uint64_t>(value)));
			else
				return value;
		}
	}	 // namespace detail

	/** @details Archive used to read non-structured binary data. */
	template<config_flags Config, typename C = char>
	class basic_input_archive
	{
	public:
		typedef input_archive_category archive_category;
		typedef std::size_t size_type;
		typedef C char_type;

	private:
		using reader_t = archive_reader<char>;

		static void throw_eof() { throw archive_error("Premature EOF"); }

	public:
		basic_input_archive() = delete;
		basic_input_archive(const basic_input_archive &) = delete;
		basic_input_archive &operator=(const basic_input_archive &) = delete;

		constexpr basic_input_archive(basic_input_archive &&) noexcept = default;
		constexpr basic_input_archive &operator=(basic_input_archive &&) noexcept = default;

		/** Initializes binary archive for reading using the specified reader.
		 * @param reader Reader used to read source data. */
		explicit basic_input_archive(archive_reader<char_type> reader) : m_reader(std::move(reader)) {}
		/** Initializes binary archive for buffer reading.
		 * @param buff Pointer to the memory buffer containing source data.
		 * @param len Size of the memory buffer. */
		basic_input_archive(const void *buff, std::size_t len)
			: basic_input_archive(archive_reader<char_type>{buff, len})
		{
		}
		/** Initializes binary archive for file reading.
		 * @param file Native file containing source data. */
		explicit basic_input_archive(system::native_file &file) : basic_input_archive(archive_reader<char_type>{file})
		{
		}
		/** Initializes binary archive for file reading.
		 * @param file Pointer to the C file containing source data.
		 * @note File must be opened in binary mode. */
		explicit basic_input_archive(FILE *file) : basic_input_archive(archive_reader<char_type>{file}) {}
		/** Initializes binary archive for stream buffer reading.
		 * @param buff Pointer to the stream buffer.
		 * @note Stream buffer must be a binary stream buffer. */
		explicit basic_input_archive(std::streambuf *buff) : basic_input_archive(archive_reader<char_type>{buff}) {}
		/** Initializes binary archive for stream reading.
		 * @param buff Stream used to read source data.
		 * @note Stream must be a binary stream. */
		explicit basic_input_archive(std::istream &is) : basic_input_archive(is.rdbuf()) {}

		/** Attempts to read a boolean (as an 8-bit integer) from the archive.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		bool try_read(bool &b, auto &&...) noexcept { return read_literal(&b); }
		/** Reads a boolean (as an 8-bit integer) from the archive.
		 * @throw archive_error On premature EOF. */
		basic_input_archive &read(bool &b, auto &&...)
		{
			if (!try_read(b)) [[unlikely]]
				throw_eof();
			return *this;
		}
		/** @copydoc read */
		bool read(std::in_place_type_t<bool>, auto &&...)
		{
			bool result;
			read(result);
			return result;
		}

		/** Attempts to read a character from the archive.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		bool try_read(char_type &c, auto &&...) noexcept
		{
			const auto result = read_literal(&c);
			c = detail::fix_endianness<Config>(c);
			return result;
		}
		/** Reads a character from the archive.
		 * @throw archive_error On premature EOF. */
		const basic_input_archive &read(char_type &c, auto &&...)
		{
			if (!try_read(c)) [[unlikely]]
				throw_eof();
			return *this;
		}
		/** @copydoc read */
		char_type read(std::in_place_type_t<char_type>, auto &&...)
		{
			bool result;
			read(result);
			return result;
		}

		/** Attempts to read an integer or floating-point number from the archive.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		template<typename I>
		bool try_read(I &i, auto &&...) noexcept
			requires(std::integral<I> || std::floating_point<I>)
		{
			const auto result = read_literal(&i);
			i = detail::fix_endianness<Config>(i);
			return result;
		}
		/** Reads an integer or floating-point number from the archive.
		 * @throw archive_error On premature EOF. */
		template<typename I>
		basic_input_archive &read(I &i, auto &&...)
			requires(std::integral<I> || std::floating_point<I>)
		{
			if (!try_read(i)) [[unlikely]]
				throw_eof();
			return *this;
		}
		/** @copydoc read */
		template<typename I>
		I read(std::in_place_type_t<I>, auto &&...)
			requires(std::integral<I> || std::floating_point<I>)
		{
			I result;
			read(result);
			return result;
		}

		/** Attempts to read a string from the archive by reading characters until null character.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		template<typename T = std::char_traits<char_type>, typename A = std::allocator<char_type>>
		bool try_read(std::basic_string<char_type, T, A> &str, auto &&...)
		{
			for (;;)
			{
				char_type c = {};
				if (!try_read(c)) [[unlikely]]
					return false;
				else if (c == '\0')
					break;
				str.append(1, c);
			}
			return true;
		}
		/** Attempts to read a string from the archive into an output iterator by reading characters until null character.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		template<std::output_iterator<char_type> I>
		bool try_read(I &value, auto &&...)
		{
			for (;; value = std::next(value))
			{
				char_type c = {};
				if (!try_read(c)) [[unlikely]]
					return false;
				else if (c == '\0')
					break;
				*value = c;
			}
			return true;
		}
		/** Attempts to read a string from the archive into an output iterator by reading characters until null
		 * character or `value == sent`.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		template<std::output_iterator<char_type> I, std::sentinel_for<I> S>
		bool try_read(I &value, S &sent, auto &&...)
		{
			for (; value != sent; value = std::next(value))
			{
				char_type c = {};
				if (!try_read(c)) [[unlikely]]
					return false;
				else if (c == '\0')
					break;
				*value = c;
			}
			return true;
		}
		/** Reads a string from the archive by reading characters until null character.
		 * @throw archive_error On premature EOF. */
		template<typename T = std::char_traits<char_type>, typename A = std::allocator<char_type>>
		basic_input_archive &read(std::basic_string<char_type, T, A> &str, auto &&...)
		{
			if (!try_read(str)) [[unlikely]]
				throw_eof();
			return *this;
		}
		/** @copydoc read */
		template<typename T = std::char_traits<char_type>, typename A = std::allocator<char_type>>
		std::basic_string<char_type, T, A> read(std::in_place_type_t<std::basic_string<char_type, T, A>>, auto &&...)
		{
			std::basic_string<char_type, T, A> result;
			read(result);
			return result;
		}
		/** Reads a string from the archive by reading characters until null character.
		 * @throw archive_error On premature EOF. */
		template<std::output_iterator<char_type> I>
		basic_input_archive &read(I &value, auto &&...)
		{
			if (!try_read(value)) [[unlikely]]
				throw_eof();
			return *this;
		}
		/** Reads a string from the archive into an output iterator by reading characters until null character or `value == sent`.
		 * @throw archive_error On premature EOF. */
		template<std::output_iterator<char_type> I, std::sentinel_for<I> S>
		basic_input_archive &read(I &value, S &sent, auto &&...)
		{
			if (!try_read(value, sent)) [[unlikely]]
				throw_eof();
			return *this;
		}

		/** Attempts to read an array of bytes from the archive.
		 * @return `true` on success, `false` on failure. Will fail on premature EOF. */
		template<std::size_t N>
		bool try_read(std::array<std::byte, N> &array, auto &&...) noexcept
		{
			return m_reader.getn(static_cast<const char *>(array.data()), array.size()) == array.size();
		}
		/** Reads an array of bytes from the archive.
		 * @throw archive_error On premature EOF. */
		template<std::size_t N>
		basic_input_archive &read(std::array<std::byte, N> &array, auto &&...)
		{
			if (!try_read(array)) [[unlikely]]
				throw_eof();
			return *this;
		}
		/** @copydoc read */
		template<std::size_t N>
		std::array<std::byte, N> read(std::in_place_type_t<std::array<std::byte, N>>, auto &&...)
		{
			std::array<std::byte, N> result;
			read(result);
			return result;
		}

		/** Attempts to deserialize an object of type `T`.
		 * @param value Value to deserialize.
		 * @param args Arguments forwarded to the deserialization function.
		 * @return `true` if deserialization was successful, `false` otherwise. */
		template<typename T, typename... Args>
		bool try_read(T &&value, Args &&...args)
		{
			try
			{
				read(std::forward<T>(value), std::forward<Args>(args)...);
				return true;
			}
			catch (archive_error &)
			{
				return false;
			}
		}
		/** Deserializes an object of type `T`.
		 * @param value Value to deserialize.
		 * @return Reference to this frame.
		 * @throw archive_exception On deserialization errors. */
		template<typename T>
		basic_input_archive &operator>>(T &&value)
		{
			return read(std::forward<T>(value));
		}
		/** @copydoc operator>>
		 * @param args Arguments forwarded to the deserialization function. */
		template<typename T, typename... Args>
		basic_input_archive &read(T &&value, Args &&...args);
		/** @brief Deserializes an instance of `T` in-place.
		 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
		 * or constructor accepting the archive frame as one of it's arguments if available.
		 * Otherwise, default-constructs & deserializes using `read(T &&)`.
		 * @param args Arguments forwarded to the deserialization function/constructor.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<typename T, typename... Args>
		T read(std::in_place_type_t<T>, Args &&...args);

		constexpr void swap(basic_input_archive &other) noexcept { m_reader.swap(other.m_reader); }
		friend constexpr void swap(basic_input_archive &a, basic_input_archive &b) noexcept { a.swap(b); }

	private:
		template<typename T>
		bool read_literal(T *value)
		{
			return m_reader.getn(std::bit_cast<char *>(value), sizeof(T)) == sizeof(T);
		}

		reader_t m_reader;
	};

	template<config_flags Config, typename C>
	template<typename T, typename... Args>
	basic_input_archive<Config, C> &basic_input_archive<Config, C>::read(T &&value, Args &&...args)
	{
		detail::do_deserialize(std::forward<T>(value), *this, std::forward<Args>(args)...);
		return *this;
	}
	template<config_flags Config, typename C>
	template<typename T, typename... Args>
	T basic_input_archive<Config, C>::read(std::in_place_type_t<T>, Args &&...args)
	{
		using archive_t = basic_input_archive<Config, C>;
		if constexpr (in_place_deserializable<T, archive_t, Args...> || std::is_constructible_v<T, archive_t &, Args...> ||
					  in_place_deserializable<T, archive_t> || std::is_constructible_v<T, archive_t &>)
		{
			return detail::do_deserialize(std::in_place_type<T>, *this, std::forward<Args>(args)...);
		}
		else
		{
			T result{};
			read(result, std::forward<Args>(args)...);
			return result;
		}
	}

	typedef basic_input_archive<little_endian> input_archive;

	/** @details Archive used to write non-structured binary data. */
	template<config_flags Config>
	class basic_output_archive
	{
	public:
		typedef output_archive_category archive_category;

	private:
	};

	typedef basic_output_archive<little_endian> output_archive;
}	 // namespace sek::serialization::binary