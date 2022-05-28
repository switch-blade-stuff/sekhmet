//
// Created by switchblade on 27/05/22.
//

#pragma once

#include <mutex>
#include <zstd.h>

#include "../math/detail/util.hpp"
#include "bswap.hpp"
#include "define.h"
#include "delegate.hpp"
#include "dynarray.hpp"
#include "thread_pool.hpp"

/*
 * ZSTD async streaming implementation.
 *
 * Compressed assets are stored as separate frames, preceding by ZSTD skipable frames.
 * Skipable frames are used to store the compressed size of the following frame,
 * as well as to identify start of an individual frame.
 * Worker threads keep reading frames until the input is exhausted.
 * */

#ifdef SEK_ARCH_LITTLE_ENDIAN
#define SWAP_LE_16(x) static_cast<std::uint16_t>(x)
#define SWAP_LE_32(x) static_cast<std::uint32_t>(x)
#define SWAP_LE_64(x) static_cast<std::uint64_t>(x)
#else
#define SWAP_LE_16(x) bswap_16(static_cast<std::uint16_t>(x))
#define SWAP_LE_32(x) bswap_32(static_cast<std::uint32_t>(x))
#define SWAP_LE_64(x) bswap_64(static_cast<std::uint64_t>(x))
#endif

#if defined(SEK_OS_WIN)
#include <malloc.h>
#define ALLOCA(n) _alloca(n)
#elif defined(__GNUC__)
#include <alloca.h>
#define ALLOCA(n) alloca(n)
#endif

namespace sek::detail
{
	/** @brief Exception thrown by worker threads when they encounter a ZSTD error. */
	class zstd_error : public std::runtime_error
	{
	public:
		zstd_error() : std::runtime_error("Unknown ZSTD error") {}
		explicit zstd_error(const char *msg) : std::runtime_error(msg) {}
		explicit zstd_error(std::size_t code) : zstd_error(ZSTD_getErrorName(code)) {}
		~zstd_error() override = default;
	};

	static auto assert_zstd_error(std::size_t code)
	{
		if (ZSTD_isError(code)) [[unlikely]]
			throw zstd_error(code);
		return code;
	}

	/** @brief Context used to synchronize multi-threaded ZSTD (de)compression.
	 * @note Each master thread receives it's own context, since (de)compression is blocking. */
	struct zstd_thread_ctx
	{
		struct buffer_t
		{
			constexpr operator ZSTD_inBuffer() const noexcept { return {.src = data, .size = size, .pos = 0}; }
			constexpr operator ZSTD_outBuffer() const noexcept { return {.dst = data, .size = size, .pos = 0}; }

			bool expand(std::size_t new_size)
			{
				data = realloc(data, size = new_size);
				return data != nullptr;
			}
			void reset() { free(data); }

			void *data = nullptr;
			std::size_t size = 0;
		};
		struct thread_task : buffer_t
		{
			constexpr thread_task() noexcept = default;
			constexpr thread_task(buffer_t buff) noexcept : buffer_t(buff) {}

			std::size_t frame_idx = 0;
		};

		struct zstd_dstream
		{
			/* Each worker thread receives it's own ZSTD_DStream. This also allows us to re-use worker state. */
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
			void decompress_frame(buffer_t &comp_buff, buffer_t &decomp_buff);

			constexpr ZSTD_DStream *operator->() noexcept { return ptr; }
			constexpr ZSTD_DStream &operator*() noexcept { return *ptr; }
			constexpr operator ZSTD_DStream *() noexcept { return ptr; }

			ZSTD_DStream *ptr;
		};

		/* Skip frames contain the frame header, specifying compressed & decompressed size of the following zstd frame.
		 * Actual ZSTD frames never exceed 32bit in size, thus 32-bit integers are used. */
		struct frame_header
		{
			std::uint32_t comp_size; /* Size of the compressed data. */
			std::uint32_t src_size;	 /* Size of the source data. */
		};

		constexpr static std::uint32_t skip_magic = 0x184d2a50;
		constexpr static std::size_t max_tasks = 32;

		static zstd_thread_ctx &get_ctx()
		{
			thread_local zstd_thread_ctx ctx;
			return ctx;
		}

		typedef delegate<std::size_t(void *, std::size_t)> read_t;
		typedef delegate<std::size_t(const void *, std::size_t)> write_t;

		[[nodiscard]] auto guard_read() { return std::lock_guard<std::mutex>{in_mtx}; }
		[[nodiscard]] auto guard_write() { return std::lock_guard<std::mutex>{out_mtx}; }

		[[nodiscard]] bool read_checked(void *dst, std::size_t n) { return read(dst, n) == n; }
		[[nodiscard]] bool write_checked(const void *src, std::size_t n) { return write(src, n) == n; }
		[[nodiscard]] bool read_frame_header(frame_header &header)
		{
			std::uint32_t buff[4];

			/* Magic & data size must match. */
			if (read_checked(buff, sizeof(buff)) && buff[0] == SWAP_LE_32(skip_magic) &&
				buff[1] == SWAP_LE_32(sizeof(frame_header))) [[likely]]
			{
				header.comp_size = SWAP_LE_32(buff[2]);
				header.src_size = SWAP_LE_32(buff[3]);
				return true;
			}
			return false;
		}
		[[nodiscard]] bool write_frame_header(const frame_header &header)
		{
			std::uint32_t buff[4];
			buff[0] = SWAP_LE_32(skip_magic);
			buff[1] = SWAP_LE_32(sizeof(frame_header));
			buff[2] = SWAP_LE_32(header.comp_size);
			buff[3] = SWAP_LE_32(header.src_size);
			return write_checked(buff, sizeof(buff));
		}

		void init(read_t r, write_t w) noexcept
		{
			in_frame = out_frame = queue_base = 0;
			task_queue.clear();
			reuse_list.clear();

			read = r;
			write = w;
		}

		buffer_t reuse_task_buffer()
		{
			buffer_t result;
			if (!reuse_list.empty())
			{
				result = reuse_list.back();
				reuse_list.pop_back();
			}
			return result;
		}
		void clear_tasks()
		{
			for (auto i = queue_base; i < task_queue.size(); ++i) task_queue[i].reset();
			for (auto &buff : reuse_list) buff.reset();
		}

		bool commit(thread_task &task)
		{
			const auto result = write_checked(task.data, task.size);
			reuse_list.push_back(task);
			return result;
		}
		bool submit(thread_task task)
		{
			/* If there is any empty space in the queue, re-use it. Otherwise, push to the end of the queue.
			 * This will break ordering of tasks within the queue, but we will attempt to commit the queue
			 * after, keeping the queue small, so it does not matter that much.
			 *
			 * Re-using empty space within the queue will reduce memory overhead. */
			if (queue_base != 0) [[likely]]
			{
				task_queue[--queue_base] = task_queue.back();
				task_queue.back() = task;
			}
			else
				task_queue.push_back(task);

			/* Attempt to commit the queue. */
			for (auto start = task_queue.size(), i = start; i > queue_base;)
				if (auto &queued = task_queue[--i]; queued.frame_idx == out_frame) [[likely]]
				{
					if (!commit(queued)) [[unlikely]]
						return false;

					/* Move out of the queue. */
					if (const auto old_base = queue_base++; i != old_base) [[likely]]
						queued = task_queue[old_base];
					/* Try to commit the queue for next frame. */
					++out_frame;
					i = start;
				}
			return true;
		}

		bool fill_decomp_frame(buffer_t &comp_buff, buffer_t &decomp_buff);
		void decompress_threaded();
		void decompress_single();

		/** Decompresses input data into the output using the thread pool.
		 * @note Decompression stops once no more input can be read.
		 * @note If thread pool's size is 1, decompresses on the main thread, ignoring the thread pool. */
		SEK_API void decompress(thread_pool &pool, read_t r, write_t w);

		std::mutex in_mtx;	/* Read synchronization. */
		std::mutex out_mtx; /* Write synchronization. */

		std::size_t in_frame;  /* Index of the next frame to be read. */
		std::size_t out_frame; /* Index of the next frame to be submitted. */

		dynarray<thread_task> task_queue;
		std::size_t queue_base;

		/* Empty task buffers are cached for reuse. */
		dynarray<buffer_t> reuse_list;

		read_t read;   /* Delegate used to read source data. */
		write_t write; /* Delegate used to write result. */
	};
}	 // namespace sek::detail