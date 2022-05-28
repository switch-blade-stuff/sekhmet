//
// Created by switchblade on 27/05/22.
//

#pragma once

#include <mutex>

#include "bswap.hpp"
#include "define.h"
#include "delegate.hpp"
#include "dynarray.hpp"
#include "thread_pool.hpp"

#ifndef ALLOCA
#if defined(SEK_OS_WIN)
#include <malloc.h>
#define ALLOCA(n) _alloca(n)
#elif defined(__GNUC__)
#include <alloca.h>
#define ALLOCA(n) alloca(n)
#endif
#endif

namespace sek
{
	/** @brief Exception thrown by worker threads when they encounter an error. */
	class SEK_API zstd_error;

	/** @brief Context used to synchronize multi-threaded ZSTD (de)compression.
	 * @note Each master thread receives it's own context, since (de)compression is blocking. */
	class zstd_thread_ctx
	{
	public:
		/** @brief Returns a per-thread context instance. */
		SEK_API static zstd_thread_ctx &instance();

	private:
		struct buffer_t
		{
			bool expand(std::size_t new_size)
			{
				data = realloc(data, size = new_size);
				return data != nullptr;
			}
			void reset() { free(data); }

			void *data = nullptr;
			std::size_t size = 0;
		};
		struct raii_buffer_t : buffer_t
		{
			constexpr raii_buffer_t() noexcept = default;
			constexpr raii_buffer_t(buffer_t buff) noexcept : buffer_t(buff) {}
			~raii_buffer_t() { reset(); }
		};
		struct thread_task : buffer_t
		{
			/* Tasks are not RAII-enabled, as they must be trivially copyable. */
			constexpr thread_task() noexcept = default;
			constexpr thread_task(raii_buffer_t &&buff, std::size_t frame) noexcept : frame_idx(frame)
			{
				data = std::exchange(buff.data, nullptr);
				size = std::exchange(buff.size, 0);
			}

			std::size_t frame_idx = 0;
		};

		struct zstd_dstream;

		/* Skip frames contain the frame header, specifying compressed & decompressed size of the following zstd frame.
		 * Actual ZSTD frames never exceed 32bit in size, thus 32-bit integers are used. */
		struct frame_header
		{
			std::uint32_t comp_size; /* Size of the compressed data. */
			std::uint32_t src_size;	 /* Size of the source data. */
		};

		constexpr static std::uint32_t skip_magic = 0x184d2a50;
		constexpr static std::size_t max_workers = 32;

		typedef delegate<std::size_t(void *, std::size_t)> read_t;
		typedef delegate<std::size_t(const void *, std::size_t)> write_t;

	public:
		/** Decompresses input data using the passed thread pool.
		 * @param pool Thread pool used for decompression.
		 * @param r Delegate used to read compressed data.
		 * @param w Delegate used to write decompressed data.
		 * @param in_size Optional size of the input data. If set to 0, will consume all available input.
		 * @note Decompression stops once no more input can be read or `in_size` limit is reached.
		 * @note If thread pool's size is 1, decompresses on the main thread, ignoring the thread pool.
		 * @throw zstd_error When ZSTD encounters an error or when the write delegate cannot fully consume decompressed data. */
		SEK_API void decompress(thread_pool &pool, read_t r, write_t w, std::size_t in_size = 0);

		/** Compresses input data at the specific compression level using the passed thread pool.
		 * @param pool Thread pool used for compression.
		 * @param r Delegate used to read source (decompressed) data.
		 * @param w Delegate used to write compressed data.
		 * @param level Compression level.
		 * @param in_size Optional size of the input data. If set to 0, will consume all available input.
		 * @note Compression stops once no more input can be read (on EOF) or `in_size` limit is reached.
		 * @note If thread pool's size is 1, decompresses on the main thread, ignoring the thread pool.
		 * @throw zstd_error When ZSTD encounters an error or when the write delegate cannot fully consume compressed data. */
		SEK_API void compress(thread_pool &pool, read_t r, write_t w, int level, std::size_t in_size = 0);

	private:
		[[nodiscard]] auto guard_read() { return std::lock_guard<std::mutex>{in_mtx}; }
		[[nodiscard]] auto guard_write() { return std::lock_guard<std::mutex>{out_mtx}; }

		[[nodiscard]] bool read_checked(void *dst, std::size_t n)
		{
			if (n > in_left) [[unlikely]]
				return false;
			else if (in_left != std::numeric_limits<std::size_t>::max()) [[likely]]
				in_left -= n;
			return read(dst, n) == n;
		}
		[[nodiscard]] bool write_checked(const void *src, std::size_t n) { return write(src, n) == n; }
		[[nodiscard]] bool read_frame_header(frame_header &header)
		{
			std::uint32_t buff[4];

			/* Magic & data size must match. */
			if (read_checked(buff, sizeof(buff)) && buff[0] == BSWAP_LE_32(skip_magic) &&
				buff[1] == BSWAP_LE_32(sizeof(frame_header))) [[likely]]
			{
				header.comp_size = BSWAP_LE_32(buff[2]);
				header.src_size = BSWAP_LE_32(buff[3]);
				return true;
			}
			return false;
		}
		[[nodiscard]] bool write_frame_header(const frame_header &header)
		{
			std::uint32_t buff[4];
			buff[0] = BSWAP_LE_32(skip_magic);
			buff[1] = BSWAP_LE_32(sizeof(frame_header));
			buff[2] = BSWAP_LE_32(header.comp_size);
			buff[3] = BSWAP_LE_32(header.src_size);
			return write_checked(buff, sizeof(buff));
		}

		void init(read_t r, write_t w, std::size_t in_size) noexcept
		{
			in_left = in_size != 0 ? in_size : std::numeric_limits<std::size_t>::max();
			in_frame = out_frame = queue_base = 0;
			task_queue.clear();
			reuse_list.clear();

			read = r;
			write = w;
		}

		void init_task_buffer(buffer_t &buff)
		{
			if (!reuse_list.empty())
			{
				buff = reuse_list.back();
				reuse_list.pop_back();
			}
			else
				buff = buffer_t{};
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
		void clear_tasks()
		{
			for (auto i = queue_base; i < task_queue.size(); ++i) task_queue[i].reset();
			for (auto &buff : reuse_list) buff.reset();
		}

		bool fill_decomp_frame(buffer_t &comp_buff, buffer_t &decomp_buff);
		void decompress_threaded();
		void decompress_single();

		void compress_threaded(int level);
		void compress_single(int level);

		template<typename F>
		void spawn_workers(thread_pool &pool, std::size_t n, F &&f)
		{
#ifdef ALLOCA
			/* Stack allocation here is fine, since std::future is not a large structure. */
			auto *wait_buf = static_cast<std::future<void> *>(ALLOCA(sizeof(std::future<void>) * n));
#else
			auto *wait_buf = static_cast<std::future<void> *>(::operator new(sizeof(std::future<void>) * total_threads));
#endif

			std::exception_ptr eptr = {};
			try
			{
				/* Schedule pool.size() workers. Some of these workers will terminate without doing anything.
				 * This is not ideal, but we can not know the amount of frames beforehand. */
				for (std::size_t i = 0; i < n; ++i) std::construct_at(wait_buf + i, pool.schedule(std::forward<F>(f)));
				/* Wait for threads to terminate. Any exceptions will be re-thrown by the worker's future. */
				for (std::size_t i = 0; i < n; ++i) wait_buf[i].get();
			}
			catch (std::bad_alloc &)
			{
				/* No cleanup is needed on `bad_alloc`. */
				throw;
			}
			catch (...)
			{
				/* Store thrown exception, since we still need to clean up allocated buffers. */
				eptr = std::current_exception();
			}

			clear_tasks();
			for (std::size_t i = 0; i < n; ++i) std::destroy_at(wait_buf + i);
#ifndef ALLOCA
			::operator delete(wait_buf);
#endif

			if (eptr) [[unlikely]]
				std::rethrow_exception(eptr);
		}

		std::mutex in_mtx;	/* Read synchronization. */
		std::mutex out_mtx; /* Write synchronization. */

		std::size_t in_left;   /* Amount of bytes left to read. */
		std::size_t in_frame;  /* Index of the next frame to be read. */
		std::size_t out_frame; /* Index of the next frame to be submitted. */

		dynarray<thread_task> task_queue;

		/* To avoid erasing & inserting tasks, committed tasks are swapped to the start of the queue.
		 * Queue base then points to the first non-empty task. */
		std::size_t queue_base;

		/* Empty task buffers are cached for reuse. */
		dynarray<buffer_t> reuse_list;

		read_t read;   /* Delegate used to read source data. */
		write_t write; /* Delegate used to write result. */
	};
}	 // namespace sek