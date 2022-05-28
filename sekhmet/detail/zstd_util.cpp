//
// Created by switchblade on 28/05/22.
//

#include "zstd_util.hpp"

#include <zstd.h>

#include "../math/detail/util.hpp"

namespace sek
{
	class zstd_error : public std::runtime_error
	{
	public:
		zstd_error() : std::runtime_error("Unknown ZSTD error") {}
		explicit zstd_error(const char *msg) : std::runtime_error(msg) {}
		explicit zstd_error(std::size_t code) : zstd_error(ZSTD_getErrorName(code)) {}
		~zstd_error() override = default;
	};

	static std::size_t assert_zstd_error(std::size_t code)
	{
		if (ZSTD_isError(code)) [[unlikely]]
			throw zstd_error(code);
		return code;
	}

	zstd_thread_ctx &zstd_thread_ctx::instance()
	{
		thread_local zstd_thread_ctx ctx;
		return ctx;
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
		void reset() { assert_zstd_error(ZSTD_DCtx_reset(ptr, ZSTD_reset_session_only)); }
		void decompress_frame(buffer_t &comp_buff, buffer_t &decomp_buff)
		{
			ZSTD_inBuffer in_buff = {.src = comp_buff.data, .size = comp_buff.size, .pos = 0};
			ZSTD_outBuffer out_buff = {.dst = decomp_buff.data, .size = decomp_buff.size, .pos = 0};

			for (;;)
			{
				const auto res = assert_zstd_error(ZSTD_decompressStream(*this, &out_buff, &in_buff));
				if (res == 0) [[likely]]
					break;
				else if (out_buff.pos < out_buff.size) [[unlikely]] /* Incomplete input frame. */
					throw zstd_error("Incomplete or invalid ZSTD frame");

				/* Not enough space in the output buffer, allocate more. */
				decomp_buff.expand(decomp_buff.size + res);
				out_buff.dst = decomp_buff.data;
				out_buff.size = decomp_buff.size;
			}

			/* Need to reset the stream after decompression, since frames are compressed independently. */
			reset();
		}

		constexpr operator ZSTD_DStream *() noexcept { return ptr; }

		ZSTD_DStream *ptr;
	};

	bool zstd_thread_ctx::fill_decomp_frame(buffer_t &comp_buff, buffer_t &decomp_buff)
	{
		frame_header header;
		/* Failed to read frame header. Even if it is not EOF yet, there is no usable frames for
		 * decompression there, so treat it as the end of compressed data. */
		if (!read_frame_header(header)) [[unlikely]]
			return false;

		/* Allocate input & output buffers. */
		if (comp_buff.expand(header.comp_size) || decomp_buff.expand(header.src_size)) [[unlikely]]
			throw std::bad_alloc();
		return read_checked(comp_buff.data, comp_buff.size);
	}
	void zstd_thread_ctx::decompress_threaded()
	{
		auto &stream = zstd_dstream::instance();

		raii_buffer_t comp_buff;
		raii_buffer_t decomp_buff;
		std::size_t frame_idx;
		for (;;)
		{

			/* Obtain a task & compressed data from the context. */
			{
				auto l = guard_read();
				init_task_buffer(decomp_buff); /* Attempt to re-use a previously committed buffer. */

				/* Failure to fill next frame means we are at the end of compressed data. */
				if (!fill_decomp_frame(comp_buff, decomp_buff)) [[unlikely]]
					break;
				frame_idx = in_frame++;
			}

			/* At this point we have a valid task & a filled compressed data buffer. */
			stream.decompress_frame(comp_buff, decomp_buff);

			/* All data of the frame has been flushed, can submit the task now. */
			{
				const auto l = guard_write();
				if (!submit(thread_task{std::move(decomp_buff), frame_idx})) [[unlikely]]
					throw zstd_error("Failed to submit decompression task");
			}
		}
	}
	void zstd_thread_ctx::decompress(thread_pool &pool, read_t r, write_t w, std::size_t frames)
	{
		/* If there is only 1 worker or frame available, do single-threaded decompression. */
		if (const auto tasks = math::min(pool.size(), max_workers, frames); tasks == 1) [[unlikely]]
			decompress_st(r, w);
		else
		{
			init(r, w);
			spawn_workers(pool, tasks, [this]() { decompress_threaded(); });
		}
	}
	void zstd_thread_ctx::decompress_st(read_t r, write_t w)
	{
		auto &stream = zstd_dstream::instance();
		init(r, w);

		raii_buffer_t comp_buff;
		raii_buffer_t decomp_buff;
		for (;;)
		{
			/* Read next frame into the compressed buffer and initialize the decompressed buffer.
			 * Failure to fill next frame means we are at the end of compressed data. */
			if (!fill_decomp_frame(comp_buff, decomp_buff)) [[unlikely]]
				break;

			/* Decompress & directly write the decompressed data to the output.
			 * There is no need to use task queue, since we will always decompress in the correct order. */
			stream.decompress_frame(comp_buff, decomp_buff);
			if (!write_checked(decomp_buff.data, decomp_buff.size)) [[unlikely]]
				throw zstd_error("Failed to write decompression result");
		}
	}

	void zstd_thread_ctx::compress_threaded(std::int32_t, std::int32_t) {}
	void zstd_thread_ctx::compress_single(std::int32_t, std::int32_t) {}
	void zstd_thread_ctx::compress(thread_pool &pool, read_t r, write_t w, int l, std::size_t in_size)
	{
		init(r, w);

		/* Clamp compression level & calculate frame size log. */
		std::int32_t level = math::min(l, 20);
		std::int32_t frame_log;
		if (in_size == 0) [[unlikely]]
		{ /* TODO: Select a pre-defined frame size. */
		}
		else
		{ /* TODO: Select a frame size based on the hint. */
		}

		/* If there is only 1 worker available, do single-threaded compression. */
		if (const auto workers = math::min(pool.size(), max_workers); workers == 1) [[unlikely]]
			compress_single(level, frame_log);
		else
			spawn_workers(pool, workers, [this, level, frame_log]() { compress_threaded(level, frame_log); });
	}
}	 // namespace sek