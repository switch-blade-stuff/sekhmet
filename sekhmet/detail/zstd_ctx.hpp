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
 * Created by switchblade on 27/05/22
 */

#pragma once

#include <mutex>

#include "bswap.hpp"
#include "compound_exception.hpp"
#include "define.h"
#include "delegate.hpp"
#include "dynarray.hpp"
#include "thread_pool.hpp"

namespace sek
{
	/** @brief Exception thrown when ZSTD (de)compression worker threads encounter an error. */
	class zstd_error : public std::runtime_error
	{
	public:
		zstd_error() : std::runtime_error("Unknown ZSTD error") {}
		explicit zstd_error(const char *msg) : std::runtime_error(msg) {}
		SEK_API explicit zstd_error(std::size_t code);
		~zstd_error() override = default;
	};

	/** @brief Compound exception used to retrieve exceptions from ZSTD worker threads. */
	class zstd_thread_error : public std::runtime_error, public compound_exception
	{
	public:
		zstd_thread_error() : std::runtime_error("ZSTD worker thread error") {}
		~zstd_thread_error() override = default;
	};

	/** @brief Context used to synchronize multi-threaded ZSTD (de)compression.
	 * @note Each master thread receives it's own context, since (de)compression is blocking. */
	class zstd_thread_ctx
	{
	public:
		typedef delegate<std::size_t(void *, std::size_t)> read_t;
		typedef delegate<std::size_t(const void *, std::size_t)> write_t;

		/** @brief Helper structure used to create a read delegate for reading a portion of a file. */
		class file_reader
		{
			static std::size_t read_impl(file_reader *r, void *dst, std::size_t n)
			{
				if (r->bytes_left < n) [[unlikely]]
					n = r->bytes_left;

				const auto total_read = fread(dst, 1, n, r->src_file);
				r->bytes_left -= total_read;
				return total_read;
			}

		public:
			file_reader() = delete;

			constexpr explicit file_reader(FILE *file, std::size_t size = std::numeric_limits<std::size_t>::max()) noexcept
				: src_file(file), bytes_left(size)
			{
			}

			[[nodiscard]] constexpr operator read_t() noexcept { return read_t{read_impl, this}; }

		private:
			FILE *src_file;
			std::size_t bytes_left;
		};
		/** @brief Helper structure used to create a write delegate for writing data to a file. */
		class file_writer
		{
			static std::size_t write_impl(file_writer *r, const void *src, std::size_t n)
			{
				return fwrite(src, 1, n, r->dst_file);
			}

		public:
			file_writer() = delete;

			constexpr explicit file_writer(FILE *file) noexcept : dst_file(file) {}

			[[nodiscard]] constexpr operator write_t() noexcept { return write_t{write_impl, this}; }

		private:
			FILE *dst_file;
		};
		/** @brief Helper structure used to create a read delegate for reading a portion of a buffer. */
		class buffer_reader
		{
			static std::size_t read_impl(buffer_reader *r, void *dst, std::size_t n)
			{
				if (r->src_pos == r->src_end) [[unlikely]]
					return 0;
				else if (const auto left = static_cast<std::size_t>(r->src_end - r->src_pos); left < n) [[unlikely]]
					n = left;
				std::copy_n(r->src_pos, n, static_cast<std::byte *>(dst));
				r->src_pos += n;
				return n;
			}

		public:
			buffer_reader() = delete;

			constexpr buffer_reader(const void *src, std::size_t size) noexcept
				: src_pos(static_cast<const std::byte *>(src)), src_end(static_cast<const std::byte *>(src) + size)
			{
			}

			[[nodiscard]] constexpr operator read_t() noexcept { return read_t{read_impl, this}; }

		private:
			const std::byte *src_pos;
			const std::byte *src_end;
		};

		/** @brief Returns a per-thread context instance. */
		SEK_API static zstd_thread_ctx &instance();

	private:
		constexpr static std::size_t min_frame_log = 19;

	public:
		/** Minimum per-thread compression frame size. Input data smaller than `min_thread_frame`
		 * will be part of the same ZSTD frame (and will be handled by the same thread).
		 * @note This minimum is only used when no frame size is specified on call to `compress`. */
		constexpr static std::size_t min_thread_frame = 1 << min_frame_log;

	private:
		struct buffer_t
		{
			bool resize(std::size_t new_size)
			{
				size = new_size;
				if (new_size > capacity)
				{
					auto new_data = realloc(data, new_size);
					if (new_data == nullptr) [[unlikely]]
						return false;
					capacity = new_size;
					data = new_data;
				}
				return true;
			}
			void reset()
			{
				free(data);
				data = nullptr;
				capacity = 0;
				size = 0;
			}

			void *data = nullptr;
			std::size_t capacity = 0;
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
		struct zstd_cstream;

		/* Skip frames contain the frame header, specifying compressed & decompressed size of the following zstd frame.
		 * Actual ZSTD frames never exceed 32bit in size, thus 32-bit integers are used. */
		struct frame_header
		{
			std::uint32_t comp_size; /* Size of the compressed data. */
			std::uint32_t src_size;	 /* Size of the source data. */
		};
		struct skip_frame
		{
			std::uint32_t magic;
			std::uint32_t size;
			frame_header header;
		};

		constexpr static std::uint32_t skip_magic = 0x184d2a50;
		constexpr static std::size_t max_workers = 32;

		static std::uint32_t get_frame_size(std::uint32_t level, std::uint32_t size_hint);

		zstd_thread_ctx();
		~zstd_thread_ctx() = default;

	public:
		zstd_thread_ctx(const zstd_thread_ctx &) = delete;
		zstd_thread_ctx &operator=(const zstd_thread_ctx &) = delete;
		zstd_thread_ctx(zstd_thread_ctx &&) = delete;
		zstd_thread_ctx &operator=(zstd_thread_ctx &&) = delete;

		/** Decompresses input data using the passed thread pool.
		 * @param pool Thread pool used for decompression.
		 * @param r Delegate used to read compressed data.
		 * @param w Delegate used to write decompressed data.
		 * @param frames Number of ZSTD frames within the input. Will be used as a hint to determine the amount
		 * of tasks to schedule. If set to 0, will spawn up to `pool.size()` or 32 tasks (whichever is smaller).
		 * @note Decompression stops once no more input can be read.
		 * @note If thread pool's size is 1, decompresses on the main thread, ignoring the thread pool.
		 * @throw zstd_thread_error Containing any exceptions thrown by worker threads (most likely to be `zstd_error`). */
		SEK_API void decompress(thread_pool &pool, read_t r, write_t w, std::size_t frames = 0);
		/** Decompresses input data single-threaded.
		 * @param r Delegate used to read compressed data.
		 * @param w Delegate used to write decompressed data.
		 * @note Decompression stops once no more input can be read.
		 * @throw zstd_error When ZSTD encounters an error or when the write delegate cannot fully consume decompressed data. */
		SEK_API void decompress_st(read_t r, write_t w);

		/** Compresses input data at the specific compression level using the passed thread pool.
		 * @param pool Thread pool used for compression.
		 * @param r Delegate used to read source (decompressed) data.
		 * @param w Delegate used to write compressed data.
		 * @param level Compression level. If set to 0, will use the implementation-defined default compression level.
		 * @param frame_size Optional size of compression frames. If set to 0, compression frame size will be deduced
		 * based on the compression level.
		 * @note Compression stops once no more input can be read (read delegate returns 0).
		 * @note Maximum compression level is 20.
		 * @note Specifying explicit frame size may reduce memory usage.
		 * @note If thread pool's size is 1, decompresses on the main thread, ignoring the thread pool.
		 * @throw zstd_thread_error Containing any exceptions thrown by worker threads (most likely to be `zstd_error`). */
		SEK_API void compress(thread_pool &pool, read_t r, write_t w, std::uint32_t level = 0, std::uint32_t frame_size = 0);
		/** Compresses input data single-threaded.
		 * @param r Delegate used to read source (decompressed) data.
		 * @param w Delegate used to write compressed data.
		 * @param level Compression level. If set to 0, will use the implementation-defined default compression level.
		 * @param frame_size Optional size of compression frames. If set to 0, compression frame size will be deduced
		 * based on the compression level.
		 * @note Compression stops once no more input can be read (read delegate returns 0).
		 * @note Maximum compression level is 20.
		 * @note Specifying explicit frame size may reduce memory usage.
		 * @throw zstd_error When ZSTD encounters an error or when the write delegate cannot fully consume compressed data. */
		SEK_API void compress_st(read_t r, write_t w, std::uint32_t level = 0, std::uint32_t frame_size = 0);

	private:
		[[nodiscard]] auto guard_read() { return std::lock_guard<std::mutex>{in_mtx}; }
		[[nodiscard]] auto guard_write() { return std::lock_guard<std::mutex>{out_mtx}; }

		[[nodiscard]] bool read_checked(void *dst, std::size_t n) { return read(dst, n) == n; }
		[[nodiscard]] bool write_checked(const void *src, std::size_t n) { return write(src, n) == n; }
		[[nodiscard]] bool read_frame_header(frame_header &header)
		{
			skip_frame frame;

			/* Magic & data size must match. */
			if (read_checked(&frame, sizeof(frame)) && frame.magic == BSWAP_LE_32(skip_magic) &&
				frame.size == BSWAP_LE_32(sizeof(frame_header))) [[likely]]
			{
				header.comp_size = BSWAP_LE_32(frame.header.comp_size);
				header.src_size = BSWAP_LE_32(frame.header.src_size);
				return true;
			}
			return false;
		}

		void init(read_t r, write_t w) noexcept
		{
			in_frame = out_frame = queue_base = 0;
			task_queue.clear();
			reuse_list.clear();

			read = r;
			write = w;
		}

		void init_task_buffer(buffer_t &buff)
		{
			/* If there are any buffers in the reuse list from previously submitted tasks, reuse an existing buffer.
			 * Otherwise, default-initialize the buffer, it will be allocated later. */
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

		bool init_decomp_frame(buffer_t &src_buff, buffer_t &dst_buff);
		void decompress_threaded();

		bool init_comp_frame(std::uint32_t frame_size, buffer_t &dst_buff, buffer_t &src_buff);
		void compress_threaded(std::uint32_t level, std::uint32_t frame_size);
		void compress_single(std::uint32_t level, std::uint32_t frame_size);

		template<typename F>
		void spawn_workers(thread_pool &pool, std::size_t n, F &&f)
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
				zstd_thread_error thread_error;
				for (std::size_t i = 0; i < n; ++i)
				{
					try
					{
						wait_buf[i].get();
					}
					catch (...)
					{
						thread_error.push_current();
					}
				}
				if (!thread_error.nested().empty()) [[unlikely]]
					eptr = std::make_exception_ptr(thread_error);
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

		std::mutex in_mtx;	/* Read synchronization. */
		std::mutex out_mtx; /* Write synchronization. */

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