/*
 * Created by switchblade on 12/09/22
 */

#pragma once

#include <string>

#include "native_file.hpp"

namespace sek::system
{
	/** @brief Structure used to preform buffered character IO operations on a native OS file.
	 *
	 * @tparam C Character type used by the file.
	 * @tparam T Character traits of `C`. */
	template<typename C, typename T = std::char_traits<C>>
	class basic_char_file : public native_file
	{
	public:
		typedef C char_type;
		typedef T traits_type;
		typedef typename T::int_type int_type;

		typedef typename native_file::native_handle_type native_handle_type;
		typedef typename native_file::seek_basis seek_basis;
		typedef typename native_file::openmode openmode;

		using native_file::append;
		using native_file::create;
		using native_file::exclusive;
		using native_file::read_only;
		using native_file::read_write;
		using native_file::sync_all_on_write;
		using native_file::truncate;
		using native_file::write_only;

		using native_file::seek_cur;
		using native_file::seek_end;
		using native_file::seek_set;

	private:
		/* Hide direct mode flag, as it is not supported by character files. */
		using native_file::direct;

	public:
		basic_char_file(const basic_char_file &) = delete;
		basic_char_file &operator=(const basic_char_file &) = delete;

		constexpr basic_char_file() noexcept = default;
		constexpr basic_char_file(basic_char_file &&) noexcept = default;
		constexpr basic_char_file &operator=(basic_char_file &&) noexcept = default;

		~basic_char_file() = default;

		/** @copydoc native_file::native_file(native_handle_type, openmode) */
		constexpr explicit basic_char_file(native_handle_type handle) : native_file(handle) {}

		/** @copydoc native_file::native_file(const path_char *, openmode)
		 * @warning Character files do not support unbuffered (`native_file::direct`) mode. */
		basic_char_file(const path_char *path, openmode mode) { open(path, mode); }
		/** @copydoc basic_char_file */
		basic_char_file(const std::filesystem::path &path, openmode mode) : basic_char_file(path.c_str(), mode) {}

		/** @copydoc native_file::open(const path_char *, openmode)
		 * @warning Character files do not support unbuffered (`native_file::direct`) mode. */
		void open(const path_char *path, openmode mode)
		{
			if (mode & native_file::direct) [[unlikely]]
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			native_file::open(path, mode);
		}
		/** @copydoc open */
		void open(const std::filesystem::path &path, openmode mode) { open(path.c_str(), mode); }

		/** @copydoc native_file::open(std::nothrow_t, const path_char *, openmode)
		 * @warning Character files do not support unbuffered (`native_file::direct`) mode. */
		expected<void, std::error_code> open(std::nothrow_t, const path_char *path, openmode mode) noexcept
		{
			if (mode & native_file::direct) [[unlikely]]
				return unexpected{std::make_error_code(std::errc::invalid_argument)};
			return native_file::open(std::nothrow, path, mode);
		}
		/** @copydoc open */
		expected<void, std::error_code> open(std::nothrow_t, const std::filesystem::path &path, openmode mode) noexcept
		{
			return open(std::nothrow, path.c_str(), mode);
		}

		/** @brief Read a single character from the input buffer, without advancing the input position.
		 * @return Character read from the file or `traits_type::eof()` if the end of file is reached.
		 * @throw std::system_error On implementation-defined system errors. */
		int_type peek() { return native_file::return_if(peek(std::nothrow)); }
		/** @copybrief peek
		 * @return Character read from the file, or an error code. */
		expected<int_type, std::error_code> peek(std::nothrow_t);

		/** @brief Reads a single character from the input buffer.
		 * @return Character read from the file or `traits_type::eof()` if the end of file is reached.
		 * @throw std::system_error On implementation-defined system errors. */
		int_type get()
		{
			if (C c; native_file::read(&c, sizeof(C)) == sizeof(C)) [[likely]]
				return T::to_int_type(c);
			return T::eof();
		}
		/** @copybrief get
		 * @return Character read from the file, or an error code. */
		expected<int_type, std::error_code> get(std::nothrow_t);

		/** @brief Reads a string from the input buffer.
		 * @param dst Destination buffer to read the characters into.
		 * @param n Size of the destination buffer.
		 * @return Amount of characters read from the file.
		 * @throw std::system_error On implementation-defined system errors. */
		std::size_t getstr(C *dst, std::size_t n) { return getstr(dst, n, T::eof()); }
		/** @copydoc getstr
		 * @param sent Sentinel character to stop reading at. If set to `traits_type::eof()`,
		 * will read all characters up to the end of file or size of the destination buffer. */
		std::size_t getstr(C *dst, std::size_t n, int_type sent)
		{
			return return_if(getstr(std::nothrow, dst, n, sent));
		}
		/** @copybrief getstr
		 * @param dst Destination buffer to read the characters into.
		 * @param n Size of the destination buffer.
		 * @return Amount of characters read from the file, or an error code. */
		expected<std::size_t, std::error_code> getstr(std::nothrow_t, C *dst, std::size_t n)
		{
			return getstr(std::nothrow, dst, n, T::eof());
		}
		/** @copydoc getstr
		 * @param sent Sentinel character to stop reading at. If set to `traits_type::eof()`,
		 * will read all characters up to the end of file or size of the destination buffer. */
		expected<std::size_t, std::error_code> getstr(std::nothrow_t, C *dst, std::size_t n, int_type sent);

		/** @brief Returns a character to the input buffer.
		 *
		 * The character is guaranteed to be returned on next subsequent read. If the file is in write mode (i.e. the
		 * last operation on it was a write), the write buffer is flushed and the file is put into the read mode.
		 *
		 * @param c Character to return to the input buffer.
		 * @throw std::system_error On implementation-defined system errors. */
		void unget(C c) { native_file::return_if(unget(std::nothrow, c)); }
		/** @brief Returns a character to the input buffer.
		 *
		 * The character is guaranteed to be returned on next subsequent read. If the file is in write mode (i.e. the
		 * last operation on it was a write), the write buffer is flushed and the file is put into the read mode.
		 *
		 * @param c Character to return to the input buffer.
		 * @return `void`, or an error code. */
		expected<void, std::error_code> unget(std::nothrow_t, C c) noexcept;
	};

	template<typename C, typename T>
	expected<typename basic_char_file<C, T>::int_type, std::error_code> basic_char_file<C, T>::peek(std::nothrow_t)
	{
		/* If there is a buffered input, return the buffered character. Otherwise get & unget. */
		if (native_file::m_reading && native_file::m_input_size - native_file::m_buffer_pos >= sizeof(C))
			return T::to_int_type(*std::bit_cast<C *>(native_file::m_buffer + native_file::m_buffer_pos));
		else
		{
			auto result = get(std::nothrow);
			if (result) [[likely]]
			{
				const auto err = unget(std::nothrow, T::to_char_type(*result));
				if (!err) [[unlikely]]
					result = unexpected{err.error()};
			}
			return result;
		}
	}
	template<typename C, typename T>
	expected<typename basic_char_file<C, T>::int_type, std::error_code> basic_char_file<C, T>::get(std::nothrow_t)
	{
		expected<std::size_t, std::error_code> read_result;
		if (C c; (read_result = native_file::read(std::nothrow, &c, sizeof(C))) && *read_result == sizeof(C)) [[likely]]
			return T::to_int_type(c);
		else if (!read_result) [[unlikely]]
			return unexpected{read_result.error()};
		return T::eof();
	}
	template<typename C, typename T>
	expected<std::size_t, std::error_code> basic_char_file<C, T>::getstr(std::nothrow_t, C *dst, std::size_t n, int_type sent)
	{
		std::size_t i = 0;
		for (;;)
		{
			if (i == n) [[unlikely]]
				break;

			const auto c = get(std::nothrow);
			if (!c) [[unlikely]]
				return unexpected{c.error()};

			dst[i++] = T::to_char_type(*c);
		}
		return i;
	}
	template<typename C, typename T>
	expected<void, std::error_code> basic_char_file<C, T>::unget(std::nothrow_t, C c) noexcept
	{
		expected<void, std::error_code> result;
		if ((!native_file::m_writing && native_file::m_buffer_pos >= sizeof(C)) || (result = flush(std::nothrow)))
		{
			/* Allocate buffer if needed. */
			if (native_file::m_buffer == nullptr) [[unlikely]]
				init_buffer(sizeof(C));

			/* If there is no buffered input, push the character into the buffer.
			 * Otherwise, decrement the read position. */
			if (native_file::m_buffer_pos != 0) [[likely]]
				native_file::m_buffer_pos -= sizeof(C);
			else
				native_file::m_input_size = sizeof(C);

			/* Always enable read mode after an unget. */
			*std::bit_cast<C *>(native_file::m_buffer + native_file::m_buffer_pos) = c;
			native_file::m_reading = true;
		}
		return result;
	}

	extern template class SEK_API_IMPORT sek::system::basic_char_file<char>;
	extern template class SEK_API_IMPORT sek::system::basic_char_file<wchar_t>;

	/** @brief Alias of `basic_char_file` that uses the `char` character type. */
	using char_file = basic_char_file<char>;
	/** @brief Alias of `basic_char_file` that uses the `wchar_t` character type. */
	using wchar_file = basic_char_file<wchar_t>;
}	 // namespace sek::system