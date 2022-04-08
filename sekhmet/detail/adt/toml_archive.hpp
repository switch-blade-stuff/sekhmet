//
// Created by switchblade on 2022-04-08.
//

#pragma once

#include "archive.hpp"

namespace sek::adt
{
	class toml_input_archive final : public basic_input_archive
	{
	public:
		using basic_input_archive::basic_input_archive;

	protected:
		SEK_API void do_read(node &n) final;
	};
	class toml_output_archive final : public basic_output_archive
	{
	public:
		using basic_output_archive::basic_output_archive;

	protected:
		SEK_API void do_write(const node &n) final;
	};
}	 // namespace sek::adt
