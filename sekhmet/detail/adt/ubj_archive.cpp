//
// Created by switchblade on 2022-04-08.
//

#include "ubj_archive.hpp"

#include <ubjf/ubjf.h>

// typedef ubjf_error (*ubjf_on_value_func)(ubjf_value value, void *udata);
// typedef char *(*ubjf_on_string_alloc_func)(size_t n, void *udata);
// typedef ubjf_error (*ubjf_on_container_begin_func)(ubjf_type container_type, int64_t fixed_size, ubjf_type
// value_type, void *udata); typedef ubjf_error (*ubjf_on_container_end_func)(void *udata);

namespace sek::adt
{
	static void throw_ubjf_error(ubjf_error error)
	{
		switch (error)
		{
			case UBJF_ERROR_ALLOC: throw std::bad_alloc();
			case UBJF_ERROR_BAD_WRITE: throw archive_error("Unable to write UBJson output");
			case UBJF_ERROR_BAD_DATA: throw archive_error("Invalid UBJson data");
			case UBJF_ERROR_BAD_TYPE: throw archive_error("Invalid UBJson type");
			case UBJF_ERROR_HIGHP: throw archive_error("High precision number support is not enabled");
			case UBJF_ERROR_UNKNOWN: throw archive_error("Unknown error");
			case UBJF_EOF: throw archive_error("End of file reached");
			default:
			{
				if (error & UBJF_ERROR_PARAM) throw archive_error("Invalid UBJF function argument");
			}
		}
	}

	void ubj_input_archive::do_read(node &n)
	{
		next_node = &n;
		if (auto err = ubjf_read_next(&state, nullptr); err != UBJF_NO_ERROR) [[unlikely]]
			throw_ubjf_error(err);
		/* TODO: Handle EOF */
	}

	ubjf_error ubj_input_archive::on_value_event(ubjf_value value, void *p) noexcept
	{
		try
		{
			auto &node = *static_cast<ubj_input_archive *>(p)->next_node;
			if (value.type == UBJF_BOOL) node.set(value.boolean);
#ifndef UBJF_NO_SPEC12 /* Type-based booleans are not supported after spec12. */
			else if (value.type & UBJF_BOOL_TYPE_MASK) [[unlikely]]
				node.set(static_cast<bool>(value.type & 1));
#endif
			else if (value.type & UBJF_INTEGER_TYPE_MASK)
				node.set(value.integer);
			else if (value.type & UBJF_FLOAT_TYPE_MASK)
				node.set(value.floating);
			else
				node.reset();
			/* TODO: Handle string types. */

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
			auto &str = static_cast<ubj_input_archive *>(p)->next_node->set<std::string>().as_string();
			str.reserve(n);
			return str.data();
		}
		catch (...)
		{
			return nullptr;
		}
	}
	void ubj_input_archive::init_state() noexcept
	{
		ubjf_read_event_info read_info = {
			.udata = this->reader,
			.read = +[](void *dest, std::size_t n, void *p) { return static_cast<reader_base *>(p)->read(dest, n); },
			.bump = +[](std::size_t n, void *p) { return static_cast<reader_base *>(p)->bump(n); },
			.peek = +[](void *p) { return static_cast<reader_base *>(p)->peek(); },
		};
		ubjf_parse_event_info parse_info = {
			.udata = this,
			.on_value = on_value_event,
			.on_string_alloc = on_string_alloc_event,
			.on_container_begin = {},
			.on_container_end = {},
		};
		ubjf_read_state_info info = {
			.read_event_info = read_info,
			.parse_event_info = parse_info,
			.highp_mode = UBJF_HIGHP_AS_STRING,
			.syntax = UBJF_SPEC_12,
		};
		if (auto err = ubjf_init_read(&state, info); err != UBJF_NO_ERROR) [[unlikely]]
			throw_ubjf_error(err);
	}

	void ubj_output_archive::do_write(const node &)
	{ /* TODO: Write UBJ */
	}
}	 // namespace sek::adt
