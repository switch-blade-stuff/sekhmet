/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 28/05/22
 */

#include "zstd_ctx.hpp"

#include <zstd.h>

#include "sekhmet/math/utility.hpp"

#include "logger.hpp"
#include <zstd_errors.h>

namespace sek::engine
{
	zstd_error::zstd_error() : std::runtime_error("Unknown ZSTD error") {}
	zstd_error::zstd_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
	zstd_error::zstd_error(const std::string &msg) : std::runtime_error(msg) {}
	zstd_error::zstd_error(const char *msg) : std::runtime_error(msg) {}
	zstd_error::zstd_error(std::size_t code) : zstd_error(ZSTD_getErrorName(code)) {}
	zstd_error::~zstd_error() = default;

	[[noreturn]] static void bad_zstd_version() noexcept
	{
		/* If ZSTD version is invalid, there is no way we can recover at any stage.
		 * The only way this could happen is if the engine was compiled with invalid version of ZSTD. */
		logger::fatal() << fmt::format("Invalid ZSTD version ({}). This should never happen "
									   "and can only be caused by incorrectly compiled engine",
									   ZSTD_versionString());
		std::abort();
	}
	static std::size_t assert_zstd_error(std::size_t code)
	{
		if (ZSTD_isError(code)) [[unlikely]]
		{
			if (const auto err = ZSTD_getErrorCode(code); err == ZSTD_error_memory_allocation) [[unlikely]]
				throw std::bad_alloc();
			else if (err == ZSTD_error_version_unsupported) [[unlikely]]
				bad_zstd_version();
			throw zstd_error(code);
		}
		return code;
	}

	zstd_thread_ctx &zstd_thread_ctx::instance()
	{
		thread_local zstd_thread_ctx ctx;
		return ctx;
	}
	zstd_thread_ctx::zstd_thread_ctx()
	{
		/* Make sure ZSTD is at least at version 1.4.0 */
		constexpr auto zstd_required_ver = (1 * 10000 + 4 * 100 + 0);
		if (ZSTD_versionNumber() < zstd_required_ver) [[unlikely]]
			bad_zstd_version();
	}

	struct zstd_thread_ctx::zstd_dstream
	{
		/* Each worker thread receives it's own ZSTD_DStream. This allows us to re-use worker state. */
		[[nodiscard]] static zstd_dstream &instance()
		{
			thread_local zstd_dstream stream;
			return stream.init(); /* ZSTD stream must be initialized before each decompression operation. */
		}

		zstd_dstream() : ptr(ZSTD_createDStream())
		{
			if (ptr == nullptr) [[unlikely]]
				throw std::bad_alloc();
		}
		~zstd_dstream() { ZSTD_freeDStream(ptr); }

		zstd_dstream &init()
		{
			assert_zstd_error(ZSTD_initDStream(ptr));
			return *this;
		}
		void reset_session() { assert_zstd_error(ZSTD_DCtx_reset(ptr, ZSTD_reset_session_only)); }
		void decompress_frame(buffer_t &src_buff, buffer_t &dst_buff)
		{
			ZSTD_inBuffer in_buff = {.src = src_buff.data, .size = src_buff.size, .pos = 0};
			ZSTD_outBuffer out_buff = {.dst = dst_buff.data, .size = dst_buff.size, .pos = 0};

			for (;;)
			{
				const auto res = assert_zstd_error(ZSTD_decompressStream(*this, &out_buff, &in_buff));
				if (res == 0) [[likely]]
					break;
				else if (out_buff.pos < out_buff.size) [[unlikely]] /* Incomplete input frame. */
					throw zstd_error("Incomplete or invalid ZSTD frame");

				/* Not enough space in the output buffer, allocate more. */
				dst_buff.resize(dst_buff.size + res);
				out_buff.dst = dst_buff.data;
				out_buff.size = dst_buff.size;
			}
			reset_session();
		}

		constexpr operator ZSTD_DStream *() noexcept { return ptr; }

		ZSTD_DStream *ptr;
	};

	bool zstd_thread_ctx::init_decomp_frame(buffer_t &src_buff, buffer_t &dst_buff)
	{
		frame_header header;
		/* Failed to read frame header. Even if it is not EOF yet, there is no usable frames for
		 * decompression there, so treat it as the end of compressed data. */
		if (!read_frame_header(header)) [[unlikely]]
			return false;

		/* Allocate input & output buffers. */
		if (!src_buff.resize(header.comp_size) || !dst_buff.resize(header.src_size)) [[unlikely]]
			throw std::bad_alloc();
		return read_checked(src_buff.data, src_buff.size);
	}
	void zstd_thread_ctx::decompress_threaded()
	{
		auto &stream = zstd_dstream::instance();

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		std::size_t frame_idx;
		for (;;)
		{
			/* Obtain a task & compressed data from the context. */
			{
				auto l = guard_read();
				init_task_buffer(dst_buff); /* Attempt to re-use a previously committed buffer. */

				/* Failure to fill next frame means we are at the end of compressed data. */
				if (!init_decomp_frame(src_buff, dst_buff)) [[unlikely]]
					break;
				frame_idx = m_in_frame++;
			}

			/* At this point we have a valid task & a filled compressed data buffer. */
			stream.decompress_frame(src_buff, dst_buff);

			/* All data of the frame has been flushed, can submit the task now. */
			{
				const auto l = guard_write();
				if (!submit(thread_task{std::move(dst_buff), frame_idx})) [[unlikely]]
					throw zstd_error("Failed to submit decompression task");
			}
		}
	}
	std::size_t zstd_thread_ctx::decompress_st(read_t r, write_t w)
	{
		auto &stream = zstd_dstream::instance();
		init(r, w);

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		for (;; ++m_out_frame)
		{
			/* Read next frame into the compressed buffer and initialize the decompressed buffer.
			 * Failure to fill next frame means we are at the end of compressed data. */
			if (!init_decomp_frame(src_buff, dst_buff)) [[unlikely]]
				break;

			/* Decompress & directly write the decompressed data to the output.
			 * There is no need to use task queue, since we will always decompress in the correct order. */
			stream.decompress_frame(src_buff, dst_buff);
			if (!write_checked(dst_buff.data, dst_buff.size)) [[unlikely]]
				throw zstd_error("Failed to write decompression result");
		}
		return m_out_frame;
	}
	std::size_t zstd_thread_ctx::decompress(thread_pool &pool, read_t r, write_t w, std::size_t frames)
	{
		/* If there is only 1 worker or frame available, do single-threaded decompression. */
		if (const auto tasks = math::min(pool.size(), max_workers, frames); tasks == 1) [[unlikely]]
			return decompress_st(r, w);
		else
		{
			init(r, w);
			spawn_workers(pool, tasks, [this]() { decompress_threaded(); });
			return m_out_frame;
		}
	}

	struct zstd_thread_ctx::zstd_cstream
	{
		/* Each worker thread receives it's own ZSTD_CStream. This allows us to re-use worker state. */
		[[nodiscard]] static zstd_cstream &instance(std::uint32_t level)
		{
			thread_local zstd_cstream stream;
			return stream.init(level); /* ZSTD stream must be initialized before each compression operation. */
		}

		zstd_cstream() : ptr(ZSTD_createCStream())
		{
			if (ptr == nullptr) [[unlikely]]
				throw std::bad_alloc();
		}
		~zstd_cstream() { ZSTD_freeCStream(ptr); }

		zstd_cstream &init(std::uint32_t level)
		{
			assert_zstd_error(ZSTD_initCStream(ptr, static_cast<int>(level)));
			return *this;
		}
		void reset_session() { assert_zstd_error(ZSTD_CCtx_reset(ptr, ZSTD_reset_session_only)); }
		void compress_frame(const buffer_t &src_buff, buffer_t &dst_buff)
		{
			ZSTD_inBuffer in_buff = {.src = src_buff.data, .size = src_buff.size, .pos = 0};
			/* Need to make some space for the skip frame header. */
			ZSTD_outBuffer out_buff = {
				.dst = static_cast<std::byte *>(dst_buff.data) + sizeof(skip_frame),
				.size = dst_buff.size - sizeof(skip_frame),
				.pos = 0,
			};

			for (;;)
			{
				const auto res = assert_zstd_error(ZSTD_compressStream2(*this, &out_buff, &in_buff, ZSTD_e_end));
				if (res == 0) [[likely]]
				{
					SEK_ASSERT(in_buff.pos == in_buff.size, "Must consume all input");

					/* On complete flush, initialize the skip frame. */
					*static_cast<skip_frame *>(dst_buff.data) = {
						.magic = BSWAP_LE_32(skip_magic),
						.size = BSWAP_LE_32(sizeof(frame_header)),
						.header =
							{
								.comp_size = BSWAP_LE_32(out_buff.pos),
								.src_size = BSWAP_LE_32(in_buff.pos),
							},
					};

					/* Make sure destination buffer's size is correct. */
					dst_buff.size = out_buff.pos + sizeof(skip_frame);
					break;
				}

				/* Not enough space in the output buffer, allocate more. */
				dst_buff.resize(dst_buff.size + res);
				out_buff.dst = static_cast<std::byte *>(dst_buff.data) + sizeof(skip_frame);
				out_buff.size = dst_buff.size - sizeof(skip_frame);
			}
			reset_session();
		}

		constexpr operator ZSTD_CStream *() noexcept { return ptr; }

		ZSTD_CStream *ptr;
	};

	std::uint32_t zstd_thread_ctx::get_frame_size(std::uint32_t level, std::uint32_t size_hint)
	{
		if (size_hint != 0)
			return size_hint;
		else
		{
			// clang-format off
			const std::uint8_t level_table[21] = {
				0, min_frame_log, min_frame_log, min_frame_log, min_frame_log + 1, min_frame_log + 1,           /* lvl 0 - 5 */
				min_frame_log + 2, min_frame_log + 2, min_frame_log + 2, min_frame_log + 2, min_frame_log + 2,  /* lvl 6 - 10 */
				min_frame_log + 3, min_frame_log + 3, min_frame_log + 3, min_frame_log + 3, min_frame_log + 3,  /* lvl 11 - 15 */
				min_frame_log + 4, min_frame_log + 4, min_frame_log + 5, min_frame_log + 5, min_frame_log + 5,  /* lvl 16 - 20 */
			};
			// clang-format on
			return static_cast<std::uint32_t>(1 << level_table[level]);
		}
	}
	bool zstd_thread_ctx::init_comp_frame(std::uint32_t frame_size, buffer_t &src_buff, buffer_t &dst_buff)
	{
		/* Allocate input & output buffers and read source data. */
		if (!dst_buff.resize(ZSTD_compressBound(frame_size)) || !src_buff.resize(frame_size)) [[unlikely]]
			throw std::bad_alloc();
		return (src_buff.size = m_read(src_buff.data, frame_size)) != 0;
	}
	void zstd_thread_ctx::compress_threaded(std::uint32_t level, std::uint32_t frame_size)
	{
		auto &stream = zstd_cstream::instance(level);

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		std::size_t frame_idx;
		for (;;)
		{
			/* Obtain a frame & read source data. */
			{
				auto l = guard_read();
				init_task_buffer(dst_buff); /* Attempt to re-use a previously committed buffer. */

				/* Attempt to read frame_size bytes from the input, actual input may be smaller.
				 * If read 0 bytes, we are at EOF. */
				if (!init_comp_frame(frame_size, src_buff, dst_buff)) [[unlikely]]
					break;
				frame_idx = m_in_frame++;
			}

			/* At this point we have a valid task & a filled source buffer. */
			stream.compress_frame(src_buff, dst_buff);

			/* Submit the compressed data to the task queue. */
			{
				const auto l = guard_write();
				if (!submit(thread_task{std::move(dst_buff), frame_idx})) [[unlikely]]
					throw zstd_error("Failed to submit compression task");
			}
		}
	}
	void zstd_thread_ctx::compress_single(std::uint32_t level, std::uint32_t frame_size)
	{
		auto &stream = zstd_cstream::instance(level);

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		for (;; ++m_out_frame)
		{
			/* Initialize both buffers & read source data up to the frame size. Actual source size may be less than the
			 * frame size. If the source size is 0 (no bytes read), we are at the end of input. */
			if (!init_comp_frame(frame_size, src_buff, dst_buff)) [[unlikely]]
				break;

			/* Compress & directly write the compressed data and frame header to the output.
			 * There is no need to use task queue, since we will always compress in the correct order. */
			stream.compress_frame(src_buff, dst_buff);
			if (!write_checked(dst_buff.data, dst_buff.size)) [[unlikely]]
				throw zstd_error("Failed to write compression result");
		}
	}
	std::size_t zstd_thread_ctx::compress(thread_pool &pool, read_t r, write_t w, std::uint32_t level, std::uint32_t frame_size)
	{
		level = level == 0 ? static_cast<std::uint32_t>(ZSTD_defaultCLevel()) : math::min(level, 20u);
		frame_size = get_frame_size(level, frame_size);
		init(r, w);

		/* If there is only 1 worker available, do single-threaded compression. */
		if (const auto workers = math::min(pool.size(), max_workers); workers == 1) [[unlikely]]
			compress_single(level, frame_size);
		else
			spawn_workers(pool, workers, [this, level, frame_size]() { compress_threaded(level, frame_size); });
		return m_out_frame;
	}
	std::size_t zstd_thread_ctx::compress_st(read_t r, write_t w, std::uint32_t level, std::uint32_t frame_size)
	{
		level = level == 0 ? static_cast<std::uint32_t>(ZSTD_defaultCLevel()) : math::min(level, 20u);
		frame_size = get_frame_size(level, frame_size);
		init(r, w);
		compress_single(level, frame_size);
		return m_out_frame;
	}

	template<typename F>
	void zstd_thread_ctx::spawn_workers(thread_pool &pool, std::size_t n, F &&f)
	{
#ifdef SEK_ALLOCA
		/* Stack allocation here is fine, since std::future is not a large structure. */
		auto *wait_buf = static_cast<std::future<void> *>(SEK_ALLOCA(sizeof(std::future<void>) * n));
#else
		auto *wait_buf = static_cast<std::future<void> *>(::operator new[](sizeof(std::future<void>) * n));
#endif

		std::exception_ptr eptr = {};
		try
		{
			/* Schedule pool.size() workers. Some of these workers will terminate without doing anything.
			 * This is not ideal, but we can not know the amount of frames beforehand. */
			for (std::size_t i = 0; i < n; ++i) std::construct_at(wait_buf + i, pool.schedule(std::forward<F>(f)));
		}
		catch (...)
		{
			/* Store thrown exception, since we still need to clean up any buffers allocated by the worker threads.
			 * Some worker threads might have had a chance to execute. */
			eptr = std::current_exception();
			goto cleanup;
		}

		/* Wait for threads to terminate. Store any exceptions to thread_error. */
		{
			std::string error_msg;
			for (std::size_t i = 0; i < n; ++i)
			{
				try
				{
					wait_buf[i].get();
				}
				catch (std::exception &e)
				{
					error_msg.append("\n\t> what(): ").append(e.what());
				}
				catch (...)
				{
					error_msg.append("\n\t> Unknown exception");
				}
			}
			if (!error_msg.empty()) [[unlikely]]
			{
				error_msg.insert(0, std::string_view{"ZSTD thread failure. Received errors:"});
				eptr = std::make_exception_ptr(zstd_error(std::move(error_msg)));
			}
		}

	cleanup:
		clear_tasks();
		for (std::size_t i = 0; i < n; ++i) std::destroy_at(wait_buf + i);

#ifndef SEK_ALLOCA
		::operator delete[](wait_buf);
#endif

		if (eptr) [[unlikely]]
			std::rethrow_exception(eptr);
	}
}	 // namespace sek::engine