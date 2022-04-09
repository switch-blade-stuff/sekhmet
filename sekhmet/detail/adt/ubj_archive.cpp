//
// Created by switchblade on 2022-04-08.
//

#include "ubj_archive.hpp"

#include <ubjf/ubjf.h>

// typedef ubjf_error (*ubjf_on_container_begin_func)(ubjf_type container_type, int64_t fixed_size, ubjf_type
// value_type, void *udata); typedef ubjf_error (*ubjf_on_container_end_func)(void *udata);

namespace sek::adt
{
	static void throw_on_ubjf_error(ubjf_error error)
	{
		switch (error)
		{
			case UBJF_NO_ERROR: [[likely]] return;
			case UBJF_ERROR_ALLOC: throw std::bad_alloc();
			case UBJF_ERROR_BAD_WRITE: throw archive_error("Unable to write UBJson output");
			case UBJF_ERROR_BAD_DATA: throw archive_error("Invalid UBJson data");
			case UBJF_ERROR_BAD_TYPE: throw archive_error("Invalid UBJson type");
			case UBJF_ERROR_HIGHP: throw archive_error("High precision number support is not enabled");
			default:
				[[unlikely]]
				{
					if (error & UBJF_ERROR_PARAM)
						throw archive_error("Invalid UBJF function argument");
					else
						throw archive_error("Unknown error");
				}
		}
	}

	ubjf_error ubj_input_archive::on_value_event(ubjf_value value, void *p) noexcept
	{
		try
		{
			auto &node = *static_cast<ubj_input_archive *>(p)->frame->data;
			if (value.type == UBJF_NOOP || value.type == UBJF_NULL) [[unlikely]]
				node.reset();
			else if (value.type == UBJF_BOOL)
				node.set(value.boolean);
#ifndef UBJF_NO_SPEC12 /* Type-based booleans are not supported after spec12. */
			else if (value.type & UBJF_BOOL_TYPE_MASK) [[unlikely]]
				node.set(static_cast<bool>(value.type & 1));
#endif
			else if (value.type & UBJF_INTEGER_TYPE_MASK)
				node.set(value.integer);
			else if (value.type & UBJF_FLOAT_TYPE_MASK)
				node.set(value.floating);

			return UBJF_NO_ERROR;
		}
		catch (std::bad_alloc &)
		{
			return UBJF_ERROR_ALLOC;
		}
		catch (...)
		{
			return UBJF_ERROR_UNKNOWN;
		}
	}
	char *ubj_input_archive::on_string_alloc_event(std::size_t n, void *p) noexcept
	{
		try
		{
			auto &str = static_cast<ubj_input_archive *>(p)->frame->data->set<std::string>().as_string();
			str.reserve(n);
			return str.data();
		}
		catch (...)
		{
			return nullptr;
		}
	}
	ubjf_error ubj_input_archive::on_container_begin_event(ubjf_type container_type,
														   std::int64_t fixed_size,
														   ubjf_type value_type,
														   void *p) noexcept
	{
		try
		{
			auto *archive = static_cast<ubj_input_archive *>(p);
			auto new_frame = new read_frame{.up = archive->frame, .data = archive->frame->data->set<>()};
		}
		catch (std::bad_alloc &)
		{
			return UBJF_ERROR_ALLOC;
		}
		catch (...)
		{
			return UBJF_ERROR_UNKNOWN;
		}
	}

	ubjf_read_state_info ubj_input_archive::make_init_info() noexcept
	{
		return {
			.read_event_info = {},
			.parse_event_info =
				{
					.udata = this,
					.on_value = on_value_event,
					.on_string_alloc = on_string_alloc_event,
					.on_container_begin = {},
					.on_container_end = {},
				},
			.highp_mode = UBJF_HIGHP_AS_STRING,
			.syntax = UBJF_SPEC_12,
		};
	}
	ubj_input_archive::ubj_input_archive(const void *buf, std::size_t n) noexcept
	{
		throw_on_ubjf_error(ubjf_init_buffer_read(&state, make_init_info(), buf, n));
		destroy_func = ubjf_destroy_buffer_read;
	}
	ubj_input_archive::ubj_input_archive(FILE *file) noexcept
	{
		throw_on_ubjf_error(ubjf_init_file_read(&state, make_init_info(), file));
		destroy_func = ubjf_destroy_file_read;
	}
	ubj_input_archive::ubj_input_archive(std::streambuf *buf) noexcept
	{
		constexpr auto read_event = [](void *dest, std::size_t n, void *data) noexcept -> std::size_t
		{
			try
			{
				auto bytes = static_cast<char *>(dest);
				auto buf = static_cast<std::streambuf *>(data);
				return static_cast<std::size_t>(buf->sgetn(bytes, static_cast<std::streamsize>(n)));
			}
			catch (...)
			{
				return 0;
			}
		};
		constexpr auto peek_event = [](void *data) noexcept -> int
		{
			try
			{
				return static_cast<std::streambuf *>(data)->sgetc();
			}
			catch (...)
			{
				return -1;
			}
		};
		constexpr auto bump_event = [](std::size_t n, void *data) noexcept -> std::size_t
		{
			try
			{
				auto buf = static_cast<std::streambuf *>(data);
				auto old = buf->pubseekoff(0, std::ios::cur, std::ios::in);
				auto abs = buf->pubseekpos(old + static_cast<std::streamoff>(n), std::ios::in);
				return static_cast<std::size_t>(abs - old);
			}
			catch (...)
			{
				return 0;
			}
		};

		auto init_info = make_init_info();
		init_info.read_event_info = ubjf_read_event_info{
			.udata = buf,
			.read = read_event,
			.bump = bump_event,
			.peek = peek_event,
		};
		throw_on_ubjf_error(ubjf_init_read(&state, init_info));
		destroy_func = ubjf_destroy_read;
	}
	void ubj_input_archive::do_read(node &n)
	{
		parse_stack.push_back({n, detail::ubj_read_frame::VALUE});
		auto error = ubjf_read_next(&state, nullptr);

		/* Destroy the frame stack before error handling can occur. */
		parse_stack.clear();

		/* Handle UBJF errors with special case for EOF. */
		if (error == UBJF_EOF) [[unlikely]]
		{
			flags = static_cast<flags_t>(flags | IS_EOF);
			if (throw_on_eof()) [[unlikely]]
				throw archive_error("End of file reached");
		}
		else
			throw_on_ubjf_error(error);
	}

	void ubj_output_archive::do_write(const node &)
	{ /* TODO: Write UBJ */
	}
}	 // namespace sek::adt
