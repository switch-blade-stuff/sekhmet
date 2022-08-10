/*
 * Created by switchblade on 28/05/22
 */

#include "zstd_ctx.hpp"

#include <zstd.h>

#include "sekhmet/expected.hpp"

#include "logger.hpp"
#include <zstd_errors.h>

namespace sek::engine
{
	/* To avoid conflicts with ZSTD errors, custom error codes start after full range of `ZSTD_ErrorCode` values. */
	enum ctx_error_t : std::uint64_t
	{
		CTX_ERROR_MASK = ((1ul << (sizeof(ZSTD_ErrorCode) * 8ul)) - 1ul) << 32,
		CTX_NO_ERROR = CTX_ERROR_MASK,
		CTX_BAD_FRAME = CTX_ERROR_MASK | 1,
		CTX_SUBMIT_FAIL = CTX_ERROR_MASK | 2,
	};

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
	[[nodiscard]] static std::string_view to_error_msg(std::uint64_t code) noexcept
	{
		if (ZSTD_isError(static_cast<std::size_t>(code)))
			return ZSTD_getErrorName(static_cast<std::size_t>(code));
		else
			switch (static_cast<ctx_error_t>(code))
			{
				case CTX_BAD_FRAME: return "Encountered invalid ZSTD frame";
				case CTX_SUBMIT_FAIL: return "Failed to submit thread task";
				default: return "Unknown error";
			}
	}

	std::uint32_t zstd_thread_ctx::get_frame_size(std::uint32_t level, std::uint32_t size_hint) noexcept
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
		[[nodiscard]] static expected<zstd_dstream *, std::uint64_t> instance()
		{
			thread_local zstd_dstream stream;
			return stream.init(); /* ZSTD stream must be initialized before each decompression operation. */
		}

		zstd_dstream() : ptr(ZSTD_createDStream()) { SEK_ASSERT(ptr != nullptr); }
		~zstd_dstream() { ZSTD_freeDStream(ptr); }

		expected<zstd_dstream *, std::uint64_t> init() noexcept
		{
			if (const auto code = ZSTD_initDStream(ptr); ZSTD_isError(code)) [[unlikely]]
				return sek::unexpected<uint64_t>{code};
			return this;
		}
		std::uint64_t decompress_frame(buffer_t &src_buff, buffer_t &dst_buff) noexcept
		{
			ZSTD_inBuffer in_buff = {.src = src_buff.data, .size = src_buff.size, .pos = 0};
			ZSTD_outBuffer out_buff = {.dst = dst_buff.data, .size = dst_buff.size, .pos = 0};

			for (;;)
			{
				const auto res = ZSTD_decompressStream(*this, &out_buff, &in_buff);
				if (ZSTD_isError(res)) [[unlikely]]
					return res;
				else if (res == 0) [[likely]]
					break;
				else if (out_buff.pos < out_buff.size) [[unlikely]] /* Incomplete input frame. */
					return CTX_BAD_FRAME;

				/* Not enough space in the output buffer, allocate more. */
				dst_buff.resize(dst_buff.size + res);
				out_buff.dst = dst_buff.data;
				out_buff.size = dst_buff.size;
			}
			return reset_session();
		}
		std::uint64_t reset_session() noexcept
		{
			const auto code = ZSTD_DCtx_reset(ptr, ZSTD_reset_session_only);
			return ZSTD_isError(code) ? code : CTX_NO_ERROR;
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
	std::uint64_t zstd_thread_ctx::decompress_threaded() noexcept
	{
		zstd_dstream *stream;
		if (auto result = zstd_dstream::instance(); result) [[likely]]
			stream = *result;
		else
			return result.error();

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		std::size_t frame_idx;
		std::uint64_t error = CTX_NO_ERROR;
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
			if ((error = stream->decompress_frame(src_buff, dst_buff)) != CTX_NO_ERROR) [[unlikely]]
				break;

			/* All data of the frame has been flushed, can submit the task now. */
			{
				const auto l = guard_write();
				if (!submit(thread_task{std::move(dst_buff), frame_idx})) [[unlikely]]
				{
					error = CTX_SUBMIT_FAIL;
					break;
				}
			}
		}
		return error;
	}
	std::uint64_t zstd_thread_ctx::decompress_single() noexcept
	{
		zstd_dstream *stream;
		if (auto result = zstd_dstream::instance(); result) [[likely]]
			stream = *result;
		else
			return result.error();

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		std::uint64_t error = CTX_NO_ERROR;
		for (;; ++m_out_frame)
		{
			/* Read next frame into the compressed buffer and initialize the decompressed buffer.
			 * Failure to fill next frame means we are at the end of compressed data. */
			if (!init_decomp_frame(src_buff, dst_buff)) [[unlikely]]
				break;

			/* Decompress & directly write the decompressed data to the output.
			 * There is no need to use task queue, since we will always decompress in the correct order. */
			if ((error = stream->decompress_frame(src_buff, dst_buff)) != CTX_NO_ERROR) [[unlikely]]
				break;
			if (!write_checked(dst_buff.data, dst_buff.size)) [[unlikely]]
			{
				error = CTX_SUBMIT_FAIL;
				break;
			}
		}
		return error;
	}

	struct zstd_thread_ctx::zstd_cstream
	{
		/* Each worker thread receives it's own ZSTD_CStream. This allows us to re-use worker state. */
		[[nodiscard]] static expected<zstd_cstream *, std::uint64_t> instance(std::uint32_t level)
		{
			thread_local zstd_cstream stream;
			return stream.init(level); /* ZSTD stream must be initialized before each compression operation. */
		}

		zstd_cstream() : ptr(ZSTD_createCStream()) { SEK_ASSERT(ptr != nullptr); }
		~zstd_cstream() { ZSTD_freeCStream(ptr); }

		expected<zstd_cstream *, std::uint64_t> init(std::uint32_t level)
		{
			if (const auto code = ZSTD_initCStream(ptr, static_cast<int>(level)); ZSTD_isError(code)) [[unlikely]]
				return sek::unexpected<std::uint64_t>{code};
			return this;
		}
		std::uint64_t reset_session()
		{
			const auto code = ZSTD_CCtx_reset(ptr, ZSTD_reset_session_only);
			return ZSTD_isError(code) ? code : CTX_NO_ERROR;
		}
		std::uint64_t compress_frame(const buffer_t &src_buff, buffer_t &dst_buff)
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
				const auto res = ZSTD_compressStream2(*this, &out_buff, &in_buff, ZSTD_e_end);
				if (ZSTD_isError(res)) [[unlikely]]
					return res;
				else if (res == 0) [[likely]]
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
			return reset_session();
		}

		constexpr operator ZSTD_CStream *() noexcept { return ptr; }

		ZSTD_CStream *ptr;
	};

	bool zstd_thread_ctx::init_comp_frame(std::uint32_t frame_size, buffer_t &src_buff, buffer_t &dst_buff)
	{
		/* Allocate input & output buffers and read source data. */
		if (!dst_buff.resize(ZSTD_compressBound(frame_size)) || !src_buff.resize(frame_size)) [[unlikely]]
			throw std::bad_alloc();
		return (src_buff.size = m_read(src_buff.data, frame_size)) != 0;
	}
	std::uint64_t zstd_thread_ctx::compress_threaded(std::uint32_t level, std::uint32_t frame_size) noexcept
	{
		zstd_cstream *stream;
		if (auto result = zstd_cstream::instance(level); result) [[likely]]
			stream = *result;
		else
			return result.error();

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		std::size_t frame_idx;
		std::uint64_t error = CTX_NO_ERROR;
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
			if ((error = stream->compress_frame(src_buff, dst_buff)) != CTX_NO_ERROR) [[unlikely]]
				break;

			/* Submit the compressed data to the task queue. */
			{
				const auto l = guard_write();
				if (!submit(thread_task{std::move(dst_buff), frame_idx})) [[unlikely]]
				{
					error = CTX_SUBMIT_FAIL;
					break;
				}
			}
		}
		return error;
	}
	std::uint64_t zstd_thread_ctx::compress_single(std::uint32_t level, std::uint32_t frame_size) noexcept
	{
		zstd_cstream *stream;
		if (auto result = zstd_cstream::instance(level); result) [[likely]]
			stream = *result;
		else
			return result.error();

		raii_buffer_t src_buff;
		raii_buffer_t dst_buff;
		std::uint64_t error = CTX_NO_ERROR;
		for (;; ++m_out_frame)
		{
			/* Initialize both buffers & read source data up to the frame size. Actual source size may be less than the
			 * frame size. If the source size is 0 (no bytes read), we are at the end of input. */
			if (!init_comp_frame(frame_size, src_buff, dst_buff)) [[unlikely]]
				break;

			/* Compress & directly write the compressed data and frame header to the output.
			 * There is no need to use task queue, since we will always compress in the correct order. */
			if ((error = stream->compress_frame(src_buff, dst_buff)) != CTX_NO_ERROR) [[unlikely]]
				break;
			if (!write_checked(dst_buff.data, dst_buff.size)) [[unlikely]]
			{
				error = CTX_SUBMIT_FAIL;
				break;
			}
		}
		return error;
	}

	template<typename F>
	void zstd_thread_ctx::spawn_workers(thread_pool &pool, std::size_t n, F &&f)
	{
#ifdef SEK_ALLOCA
		/* Stack allocation here is fine, since std::future is not a large structure. */
		auto *wait_buf = static_cast<std::future<std::uint64_t> *>(SEK_ALLOCA(sizeof(std::future<std::uint64_t>) * n));
#else
		auto *wait_buf = static_cast<std::future<std::uint64_t> *>(::operator new[](sizeof(std::future<std::uint64_t>) * n));
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
				if (const auto result = wait_buf[i].get(); result != CTX_NO_ERROR) [[likely]]
					error_msg.append("\n\t> ").append(to_error_msg(result));
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

	static void assert_error(std::uint64_t code)
	{
		if (code != CTX_NO_ERROR) [[unlikely]]
			throw zstd_error(to_error_msg(code).data());
	}

	std::size_t zstd_thread_ctx::decompress(thread_pool &pool, read_t r, write_t w, std::size_t frames)
	{
		init(r, w);

		/* If there is only 1 worker or frame available, do single-threaded decompression. */
		if (const auto tasks = std::min(std::min(pool.size(), max_workers), frames); tasks > 1) [[unlikely]]
			spawn_workers(pool, tasks, [this]() { return decompress_threaded(); });
		else
			assert_error(decompress_single());
		return m_out_frame;
	}
	std::size_t zstd_thread_ctx::decompress_st(read_t r, write_t w)
	{
		init(r, w);
		assert_error(decompress_single());
		return m_out_frame;
	}

	std::size_t zstd_thread_ctx::compress(thread_pool &pool, read_t r, write_t w, std::uint32_t level, std::uint32_t frame_size)
	{
		level = level == 0 ? static_cast<std::uint32_t>(ZSTD_defaultCLevel()) : std::min(level, 20u);
		frame_size = get_frame_size(level, frame_size);
		init(r, w);

		/* If there is only 1 worker available, do single-threaded compression. */
		if (const auto workers = std::min(pool.size(), max_workers); workers > 1) [[unlikely]]
			spawn_workers(pool, workers, [this, level, frame_size]() { return compress_threaded(level, frame_size); });
		else
			assert_error(compress_single(level, frame_size));
		return m_out_frame;
	}
	std::size_t zstd_thread_ctx::compress_st(read_t r, write_t w, std::uint32_t level, std::uint32_t frame_size)
	{
		level = level == 0 ? static_cast<std::uint32_t>(ZSTD_defaultCLevel()) : std::min(level, 20u);
		frame_size = get_frame_size(level, frame_size);
		init(r, w);

		assert_error(compress_single(level, frame_size));
		return m_out_frame;
	}
}	 // namespace sek::engine