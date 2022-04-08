//
// Created by switchblade on 2022-04-08.
//

#pragma once

#include "archive.hpp"
#include <ubjf/ubjf.h>

namespace sek::adt
{
	class ubj_input_archive final : public basic_input_archive
	{
	public:
		constexpr ubj_input_archive() noexcept = default;
		constexpr ubj_input_archive(ubj_input_archive &&other) noexcept
			: basic_input_archive(std::move(other)), state(std::exchange(other.state, {}))
		{
		}
		constexpr ubj_input_archive &operator=(ubj_input_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~ubj_input_archive() { ubjf_destroy_read(&state); }

		/** Initializes a UBJson archive from a raw memory buffer. */
		ubj_input_archive(const void *buf, std::size_t n) noexcept : basic_input_archive(buf, n) { init_state(); }
		/** Initializes a UBJson archive from a `FILE *`. */
		explicit ubj_input_archive(FILE *file) noexcept : basic_input_archive(file) { init_state(); }
		/** Initializes a UBJson archive from an input buffer. */
		explicit ubj_input_archive(std::streambuf *buf) noexcept : basic_input_archive(buf) { init_state(); }
		/** Initializes a UBJson archive from an input stream. */
		explicit ubj_input_archive(std::istream &is) noexcept : basic_input_archive(is) { init_state(); }

		constexpr void swap(ubj_input_archive &other) noexcept
		{
			std::swap(state, other.state);
			basic_input_archive::swap(other);
		}
		friend constexpr void swap(ubj_input_archive &a, ubj_input_archive &b) noexcept { a.swap(b); }

	protected:
		SEK_API void do_read(node &n) final;

	private:
		static ubjf_error on_value_event(ubjf_value value, void *udata) noexcept;
		static char *on_string_alloc_event(std::size_t n, void *udata) noexcept;

		SEK_API void init_state() noexcept;

		node *next_node = nullptr;
		ubjf_read_state state = {};
	};
	class ubj_output_archive final : public basic_output_archive
	{
	public:
		using basic_output_archive::basic_output_archive;

	protected:
		SEK_API void do_write(const node &n) final;

		ubjf_write_state state;
	};
}	 // namespace sek::adt
