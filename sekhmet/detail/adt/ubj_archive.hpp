//
// Created by switchblade on 2022-04-08.
//

#pragma once

#include "archive.hpp"
#include <ubjf/ubjf.h>

namespace sek::adt
{
	namespace detail
	{
		struct ubj_read_frame
		{
			enum mode_t
			{
				/* data = target value node. */
				VALUE,
				/* data = parent sequence node. */
				ARRAY,
				/* data = parent table node. */
				OBJECT_KEY,
			};

			node &data;
			mode_t mode;
		};
	}

	class ubj_input_archive final : public basic_input_archive
	{
	public:
		SEK_ADT_NODE_CONSTEXPR_VECTOR ubj_input_archive(ubj_input_archive &&other) noexcept
			: basic_input_archive(std::move(other)),
			  destroy_func(std::exchange(other.destroy_func, {})),
			  parse_stack(std::move(other.parse_stack)),
			  state(std::exchange(other.state, {}))
		{
		}
		SEK_ADT_NODE_CONSTEXPR_VECTOR ubj_input_archive &operator=(ubj_input_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~ubj_input_archive() final
		{
			if (destroy_func) [[likely]]
				destroy_func(&state);
		}

		/** Initializes a UBJson archive from a raw memory buffer. */
		SEK_API ubj_input_archive(const void *buf, std::size_t n) noexcept;
		/** Initializes a UBJson archive from a `FILE *`. */
		SEK_API explicit ubj_input_archive(FILE *file) noexcept;
		/** Initializes a UBJson archive from an input buffer. */
		SEK_API explicit ubj_input_archive(std::streambuf *buf) noexcept;
		/** Initializes a UBJson archive from an input stream. */
		explicit ubj_input_archive(std::istream &is) noexcept : ubj_input_archive(is.rdbuf()) {}

		constexpr void swap(ubj_input_archive &other) noexcept
		{
			using std::swap;
			swap(destroy_func, other.destroy_func);
			swap(parse_stack, other.parse_stack);
			swap(state, other.state);
			basic_input_archive::swap(other);
		}
		friend constexpr void swap(ubj_input_archive &a, ubj_input_archive &b) noexcept { a.swap(b); }

	protected:
		SEK_API void do_read(node &n) final;

	private:
		static ubjf_error on_value_event(ubjf_value, void *) noexcept;
		static char *on_string_alloc_event(std::size_t, void *) noexcept;
		ubjf_error on_container_begin_event(ubjf_type, std::int64_t, ubjf_type, void *) noexcept;

		ubjf_read_state_info make_init_info() noexcept;

		void (*destroy_func)(ubjf_read_state *) = nullptr;
		std::vector<detail::ubj_read_frame> parse_stack = {};
		ubjf_read_state state = {};
	};
}	 // namespace sek::adt
