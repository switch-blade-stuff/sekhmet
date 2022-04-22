//
// Created by switchblade on 2022-04-19.
//

#pragma once

#include <iostream>

#include "common.hpp"
#include <rapidjson/prettywriter.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>

namespace sek::serialization::json
{
	namespace detail
	{
		using namespace sek::serialization::detail;

		constexpr static std::size_t rj_allocator_page_size = 512;

		using base_archive = detail::json_archive_base<char, false, false>;
		using rj_encoding = rapidjson::UTF8<>;

		struct rj_allocator : detail::basic_pool_resource<rj_allocator_page_size>
		{
			using base_pool = detail::basic_pool_resource<rj_allocator_page_size>;

			SEK_FORCE_INLINE constexpr static void Free(void *) {}

			constexpr static bool kNeedFree = false;

			constexpr rj_allocator() noexcept = default;
			constexpr explicit rj_allocator(std::pmr::memory_resource *res) noexcept : basic_pool_resource(res) {}

			SEK_FORCE_INLINE void *Malloc(std::size_t n) { return base_pool::allocate(n); }
			SEK_FORCE_INLINE void *Realloc(void *old, std::size_t old_n, std::size_t n) noexcept
			{
				return base_pool::reallocate(old, old_n, n);
			}
		};
	}	 // namespace detail

	/** @details Archive used to read Json data. Internally uses the RapidJSON library.
	 *
	 * The archive itself does not do any deserialization, instead deserialization is done by special archive frames,
	 * which represent a Json object or array. These frames are then passed to deserialization functions
	 * of serializable types. */
	class input_archive : detail::base_archive
	{
		using base_t = detail::base_archive;

	public:
		typedef typename base_t::read_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;
		typedef typename archive_frame::char_type char_type;
		typedef typename archive_frame::size_type size_type;

	private:
		using rj_parser = rapidjson::GenericReader<detail::rj_encoding, detail::rj_encoding, detail::rj_allocator>;

		struct rj_buffer_reader
		{
			using Ch = char_type;

			constexpr rj_buffer_reader(const char_type *buff, size_type n) noexcept
				: begin(buff), curr(begin), end(begin + n)
			{
			}

			constexpr char_type Peek() const { return curr == end ? '\0' : *curr; }
			constexpr char_type Take() { return curr == end ? '\0' : *curr++; }
			constexpr size_type Tell() const { return static_cast<size_type>(curr - begin); }

			[[noreturn]] void Put(char_type) { SEK_NEVER_REACHED; }
			[[noreturn]] void Flush() { SEK_NEVER_REACHED; }
			[[noreturn]] char_type *PutBegin() { SEK_NEVER_REACHED; }
			[[noreturn]] size_type PutEnd(char_type *) { SEK_NEVER_REACHED; }

			const char_type *begin;
			const char_type *curr;
			const char_type *end;
		};
		struct rj_file_reader
		{
			using Ch = char_type;

			constexpr explicit rj_file_reader(FILE *file) noexcept : file(file) {}

			char_type Peek() const
			{
				auto c = ungetc(getc(file), file);
				return c == EOF ? '\0' : static_cast<char_type>(c);
			}
			char_type Take()
			{
				auto c = getc(file);
				return c == EOF ? '\0' : static_cast<char_type>(c);
			}
			size_type Tell() const
			{
#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
				auto pos = ftello64(file);
#else
				auto pos = ftello(file);
#endif
#elif defined(SEK_OS_WIN)
				auto pos = _ftelli64(file);
#else
				auto pos = ftell(file);
#endif

				if (pos >= 0) [[likely]]
					return static_cast<size_type>(pos);
				else
					return static_cast<size_type>(-1LL);
			}

			[[noreturn]] void Put(char_type) { SEK_NEVER_REACHED; }
			[[noreturn]] void Flush() { SEK_NEVER_REACHED; }
			[[noreturn]] char_type *PutBegin() { SEK_NEVER_REACHED; }
			[[noreturn]] size_type PutEnd(char_type *) { SEK_NEVER_REACHED; }

			FILE *file;
		};
		struct rj_streambuf_reader
		{
			using Ch = char_type;

			constexpr explicit rj_streambuf_reader(std::streambuf *buff) noexcept : buff(buff) {}

			char_type Peek() const
			{
				auto c = buff->sgetc();
				return c == std::streambuf::traits_type::eof() ? '\0' : static_cast<char_type>(c);
			}
			char_type Take()
			{
				auto c = buff->sbumpc();
				return c == std::streambuf::traits_type::eof() ? '\0' : static_cast<char_type>(c);
			}
			size_type Tell() const { return static_cast<size_type>(buff->pubseekoff(0, std::ios::cur, std::ios::in)); }

			[[noreturn]] void Put(char_type) { SEK_NEVER_REACHED; }
			[[noreturn]] void Flush() { SEK_NEVER_REACHED; }
			[[noreturn]] char_type *PutBegin() { SEK_NEVER_REACHED; }
			[[noreturn]] size_type PutEnd(char_type *) { SEK_NEVER_REACHED; }

			std::streambuf *buff;
		};

		struct rj_event_handler : base_t::parser_base
		{
			constexpr explicit rj_event_handler(json_archive_base &parent) noexcept : parser_base(parent) {}

			SEK_FORCE_INLINE bool Null() { return parser_base::on_null(); }
			SEK_FORCE_INLINE bool Bool(bool b) { return parser_base::on_bool(b); }

			SEK_FORCE_INLINE bool Int(int i) { return parser_base::on_int(i); }
			SEK_FORCE_INLINE bool Uint(unsigned i) { return parser_base::on_int(i); }
			SEK_FORCE_INLINE bool Int64(std::int64_t i) { return parser_base::on_int(i); }
			SEK_FORCE_INLINE bool Uint64(std::uint64_t i) { return parser_base::on_int(i); }
			SEK_FORCE_INLINE bool Double(double d) { return parser_base::on_float(d); }

			SEK_FORCE_INLINE bool String(const char_type *str, size_type len, bool)
			{
				return parser_base::on_string_copy(str, len);
			}

			SEK_FORCE_INLINE bool StartObject() { return parser_base::on_object_start(); }
			SEK_FORCE_INLINE bool Key(const char_type *str, size_type len, bool)
			{
				return parser_base::on_object_key_copy(str, len);
			}
			SEK_FORCE_INLINE bool EndObject(size_type n) { return parser_base::on_object_end(n); }

			SEK_FORCE_INLINE bool StartArray() { return parser_base::on_array_start(); }
			SEK_FORCE_INLINE bool EndArray(size_type n) { return parser_base::on_array_end(n); }

			[[noreturn]] bool RawNumber(const char_type *, size_type, bool) { SEK_NEVER_REACHED; }
		};

	public:
		input_archive() = delete;
		input_archive(const input_archive &) = delete;
		input_archive &operator=(const input_archive &) = delete;

		constexpr input_archive(input_archive &&) noexcept = default;
		constexpr input_archive &operator=(input_archive &&other) noexcept
		{
			base_t::operator=(std::forward<input_archive>(other));
			return *this;
		}

		/** Reads Json from a character buffer.
		 * @param buff Pointer to the character buffer containing Json data.
		 * @param len Size of the character buffer. */
		input_archive(const char_type *buff, std::size_t len)
			: input_archive(buff, len, std::pmr::get_default_resource())
		{
		}
		/** @copydoc input_archive
		 * @param res PMR memory resource used for internal allocation. */
		input_archive(const char_type *buff, std::size_t len, std::pmr::memory_resource *res) : base_t(res)
		{
			parse(buff, len);
		}
		/** Reads Json from a file.
		 * @param file Pointer to the Json file. */
		explicit input_archive(FILE *file) : input_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc input_archive
		 * @param res Memory resource used for internal allocation. */
		input_archive(FILE *file, std::pmr::memory_resource *res) : base_t(res) { parse(file); }
		/** Reads Json from a stream buffer.
		 * @param buff Pointer to the stream buffer. */
		explicit input_archive(std::streambuf *buff) : input_archive(buff, std::pmr::get_default_resource()) {}
		/** @copydoc input_archive
		 * @param res Memory resource used for internal allocation. */
		input_archive(std::streambuf *buff, std::pmr::memory_resource *res) : base_t(res) { parse(buff); }
		/** Reads Json from an input stream.
		 * @param is Reference to the input stream. */
		explicit input_archive(std::istream &is) : input_archive(is.rdbuf()) {}
		/** @copydoc input_archive
		 * @param res Memory resource used for internal allocation. */
		input_archive(std::istream &is, std::pmr::memory_resource *res) : input_archive(is.rdbuf(), res) {}

		/** Attempts to deserialize the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return true if deserialization was successful, false otherwise. */
		template<typename T>
		bool try_read(T &&value)
		{
			return base_t::do_try_read(std::forward<T>(value));
		}
		/** Deserializes the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return Reference to this archive.
		 * @throw archive_error On deserialization errors. */
		template<typename T>
		input_archive &read(T &&value)
		{
			base_t::do_read(std::forward<T>(value));
			return *this;
		}
		/** @copydoc read */
		template<typename T>
		input_archive &operator>>(T &&value)
		{
			return read(std::forward<T>(value));
		}
		/** Deserializes an instance of `T` from the top-level Json entry of the archive.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<std::default_initializable T>
		T read() const
		{
			T result;
			read(result);
			return result;
		}

		constexpr void swap(input_archive &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(input_archive &a, input_archive &b) noexcept { a.swap(b); }

	private:
		void parse(auto &reader)
		{
			rj_event_handler handler{*this};
			detail::rj_allocator allocator{base_t::upstream};
			rj_parser parser{&allocator};

			if (!parser.Parse(reader, handler)) [[unlikely]]
			{
				std::string error_msg = "Json parser error at ";
				error_msg.append(std::to_string(parser.GetErrorOffset()));
				throw archive_error(error_msg);
			}
		}
		void parse(const char_type *data, std::size_t n)
		{
			rj_buffer_reader reader{data, n};
			parse(reader);
		}
		void parse(FILE *file)
		{
			rj_file_reader reader{file};
			parse(reader);
		}
		void parse(std::streambuf *buff)
		{
			rj_streambuf_reader reader{buff};
			parse(reader);
		}
	};

	static_assert(serialization::input_archive<input_archive::archive_frame, bool>);
	static_assert(serialization::input_archive<input_archive::archive_frame, char>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::uint8_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int8_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int16_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int32_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int64_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, float>);
	static_assert(serialization::input_archive<input_archive::archive_frame, double>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::string>);
	static_assert(serialization::container_like_archive<input_archive::archive_frame>);

	typedef int config_flags;

	/** Enables pretty-printing for Json output. */
	constexpr config_flags pretty_print = 1;
	/** If pretty printing is enabled, writes arrays on a single line. */
	constexpr config_flags inline_arrays = 2;
	/** Enables extended floating-point values (NaN, inf). */
	constexpr config_flags extended_fp = 4;
	constexpr config_flags no_flags = 0;

	/** @details Archive used to write Json data. Internally uses the RapidJSON library.
	 *
	 * The archive itself does not do any serialization, instead serialization is done by archive frames,
	 * which represent a Json object or array. These frames are then passed to serialization functions
	 * of serializable types.
	 *
	 * @tparam Config Configuration flags used for the archive.
	 * @tparam IndentC Indentation character used if `pretty_print` flag is present.
	 * @tparam IndentN Amount of indentation characters used if `pretty_print` flag is present. */
	template<config_flags Config, char IndentC = ' ', std::size_t IndentN = 4>
	class basic_output_archive : detail::base_archive
	{
		using base_t = detail::base_archive;

	public:
		typedef typename base_t::write_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;
		typedef typename archive_frame::char_type char_type;
		typedef typename archive_frame::size_type size_type;

	private:
		struct rj_writer
		{
			using Ch = char_type;

			void Put(char_type c) { put_func(this, c); }
			void Flush() { flush_func(this); }
			size_type Tell() const { return tell_func(this); }

			[[noreturn]] char_type Peek() const { SEK_NEVER_REACHED; }
			[[noreturn]] char_type Take() { SEK_NEVER_REACHED; }
			[[noreturn]] char_type *PutBegin() { SEK_NEVER_REACHED; }
			[[noreturn]] size_type PutEnd(char_type *) { SEK_NEVER_REACHED; }

			void (*put_func)(void *, char_type);
			void (*flush_func)(void *);
			size_type (*tell_func)(const void *);
		};
		struct rj_buffer_writer final : rj_writer
		{
			constexpr rj_buffer_writer(char_type *buff, size_type n) noexcept : begin(buff), curr(begin), end(begin + n)
			{
				rj_writer::put_func = +[](void *p, char_type c)
				{
					auto writer = static_cast<rj_buffer_writer *>(p);
					if (writer->curr < writer->end) [[likely]]
						*writer->curr++ = c;
				};
				rj_writer::flush_func = +[](void *) {};
				rj_writer::tell_func = +[](const void *p)
				{
					auto writer = static_cast<const rj_buffer_writer *>(p);
					return static_cast<size_type>(writer->curr - writer->begin);
				};
			}

			char_type *begin;
			char_type *curr;
			char_type *end;
		};
		struct rj_file_writer final : rj_writer
		{
			constexpr explicit rj_file_writer(FILE *file) noexcept : file(file)
			{
				rj_writer::put_func = +[](void *p, char_type c)
				{
					auto writer = static_cast<rj_file_writer *>(p);
					fputc(static_cast<int>(c), writer->file);
				};
				rj_writer::flush_func = +[](void *p)
				{
					auto writer = static_cast<rj_file_writer *>(p);
					fflush(writer->file);
				};
				rj_writer::tell_func = +[](const void *p)
				{
					auto writer = static_cast<const rj_file_writer *>(p);
#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
					auto pos = ftello64(writer->file);
#else
					auto pos = ftello(writer->file);
#endif
#elif defined(SEK_OS_WIN)
					auto pos = _ftelli64(writer->file);
#else
					auto pos = ftell(writer->file);
#endif

					if (pos >= 0) [[likely]]
						return static_cast<size_type>(pos);
					else
						return static_cast<size_type>(-1LL);
				};
			}

			FILE *file;
		};
		struct rj_streambuf_writer final : rj_writer
		{
			constexpr explicit rj_streambuf_writer(std::streambuf *buff) noexcept : buff(buff)
			{

				rj_writer::put_func = +[](void *p, char_type c)
				{ static_cast<rj_streambuf_writer *>(p)->buff->sputc(c); };
				rj_writer::flush_func = +[](void *p) { static_cast<rj_streambuf_writer *>(p)->buff->pubsync(); };
				rj_writer::tell_func = +[](const void *p)
				{
					auto writer = static_cast<const rj_streambuf_writer *>(p);
					return static_cast<size_type>(writer->buff->pubseekoff(0, std::ios::cur, std::ios::in));
				};
			}

			std::streambuf *buff;
		};

		constexpr static auto rj_emitter_flags = (Config & extended_fp) == extended_fp ? rapidjson::kWriteNanAndInfFlag :
																						   rapidjson::kWriteDefaultFlags;
		using rj_emitter_base =
			std::conditional_t<(Config & pretty_print) == pretty_print,
							   rapidjson::PrettyWriter<rj_writer, detail::rj_encoding, detail::rj_encoding, detail::rj_allocator, rj_emitter_flags>,
							   rapidjson::Writer<rj_writer, detail::rj_encoding, detail::rj_encoding, detail::rj_allocator, rj_emitter_flags>>;

		struct rj_emitter : rj_emitter_base
		{
			struct frame_t
			{
				std::size_t size;
			};

			rj_emitter(rj_writer &writer, detail::rj_allocator &allocator) : rj_emitter_base(writer, &allocator)
			{
				if constexpr ((Config & pretty_print) == pretty_print)
				{
					rj_emitter_base::SetIndent(IndentC, static_cast<unsigned>(IndentN));
					if constexpr ((Config & inline_arrays) == inline_arrays)
						rj_emitter_base::SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
				}
			}

			SEK_FORCE_INLINE void on_null() { rj_emitter_base::Null(); }
			SEK_FORCE_INLINE void on_bool(bool b) { rj_emitter_base::Bool(b); }
			SEK_FORCE_INLINE void on_int(entry_type type, std::intmax_t value)
			{
				switch (type)
				{
					case entry_type::INT_S8:
					case entry_type::INT_S16:
					case entry_type::INT_S32: rj_emitter_base::Int(static_cast<int>(value)); break;
					case entry_type::INT_S64: rj_emitter_base::Int64(static_cast<std::int64_t>(value)); break;
					default: [[unlikely]] SEK_NEVER_REACHED;
				}
			}
			SEK_FORCE_INLINE void on_uint(entry_type type, std::uintmax_t value)
			{
				switch (type)
				{
					case entry_type::INT_U8:
					case entry_type::INT_U16:
					case entry_type::INT_U32: rj_emitter_base::Uint(static_cast<unsigned>(value)); break;
					case entry_type::INT_U64: rj_emitter_base::Uint64(static_cast<std::uint64_t>(value)); break;
					default: [[unlikely]] SEK_NEVER_REACHED;
				}
			}
			SEK_FORCE_INLINE void on_float32(float value) { on_float64(static_cast<double>(value)); }
			SEK_FORCE_INLINE void on_float64(double value) { rj_emitter_base::Double(value); }
			SEK_FORCE_INLINE void on_string(const char_type *str, std::size_t size)
			{
				rj_emitter_base::String(str, static_cast<unsigned>(size));
			}
			SEK_FORCE_INLINE void on_array_start(std::size_t size, entry_type)
			{
				frame.size = size;
				rj_emitter_base::StartArray();
			}
			SEK_FORCE_INLINE void on_array_end() { rj_emitter_base::EndArray(static_cast<unsigned>(frame.size)); }
			SEK_FORCE_INLINE void on_object_start(std::size_t size, entry_type)
			{
				frame.size = size;
				rj_emitter_base::StartObject();
			}
			SEK_FORCE_INLINE void on_object_key(const char_type *str, std::size_t size)
			{
				rj_emitter_base::Key(str, static_cast<unsigned>(size));
			}
			SEK_FORCE_INLINE void on_object_end() { rj_emitter_base::EndObject(static_cast<unsigned>(frame.size)); }

			constexpr frame_t enter_frame() { return frame; }
			constexpr void exit_frame(frame_t old) { frame = old; }

			frame_t frame;
		};

	public:
		basic_output_archive() = delete;
		basic_output_archive(const basic_output_archive &) = delete;
		basic_output_archive &operator=(const basic_output_archive &) = delete;

		constexpr basic_output_archive(basic_output_archive &&other) noexcept
			: base_t(std::forward<basic_output_archive>(other))
		{
		}
		constexpr basic_output_archive &operator=(basic_output_archive &&other) noexcept
		{
			base_t::operator=(std::forward<basic_output_archive>(other));
			return *this;
		}

		/** Initialized output archive for buffer writing.
		 * @param buff Memory buffer to write Json data to.
		 * @param size Size of the character buffer. */
		basic_output_archive(char_type *buff, size_type size)
			: basic_output_archive(buff, size, std::pmr::get_default_resource())
		{
		}
		/** @copydoc output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(char_type *buff, size_type size, std::pmr::memory_resource *res)
			: base_t(res), buffer_writer(buff, size)
		{
		}
		/** Initialized output archive for file writing.
		 * @param file File to write Json data to. */
		explicit basic_output_archive(FILE *file) : basic_output_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(FILE *file, std::pmr::memory_resource *res) : base_t(res), file_writer(file) {}
		/** Initialized output archive for stream buffer writing.
		 * @param buff Stream buffer to write Json data to. */
		explicit basic_output_archive(std::streambuf *buff)
			: basic_output_archive(buff, std::pmr::get_default_resource())
		{
		}
		/** @copydoc output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(std::streambuf *buff, std::pmr::memory_resource *res) : base_t(res), streambuf_writer(buff)
		{
		}
		/** Initialized output archive for stream writing.
		 * @param os Output stream to write Json data to. */
		explicit basic_output_archive(std::ostream &os) : basic_output_archive(os.rdbuf()) {}
		/** @copydoc output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(std::ostream &os, std::pmr::memory_resource *res) : basic_output_archive(os.rdbuf(), res)
		{
		}

		~basic_output_archive() { flush_impl(); }

		/** Serializes the forwarded value to Json. Flushes previous uncommitted state.
		 * @param value Value to serialize as Json.
		 * @return Reference to this archive.
		 * @note Serialized data is kept inside the archive's internal state and will be written to the output once the
		 * archive is destroyed or `flush` is called. */
		template<typename T>
		basic_output_archive &write(T &&value)
		{
			/* Flush uncommitted changes before initializing a new emit tree. */
			flush();
			base_t::do_write(std::forward<T>(value));
			return *this;
		}
		/** @copydoc write */
		template<typename T>
		basic_output_archive &operator<<(T &&value)
		{
			return write(std::forward<T>(value));
		}

		/** Flushes the internal state & writes Json to the output. */
		void flush()
		{
			flush_impl();
			base_t::reset();
		}

		constexpr void swap(basic_output_archive &other) noexcept
		{
			base_t::swap(other);
			std::swap(writer_padding, other.writer_padding);
		}
		friend constexpr void swap(basic_output_archive &a, basic_output_archive &b) noexcept { a.swap(b); }

	private:
		void flush_impl()
		{
			detail::rj_allocator allocator{base_t::upstream};
			rj_emitter emitter{writer, allocator};
			base_t::do_flush(emitter);
		}

		union
		{
			std::byte writer_padding[sizeof(rj_buffer_writer)] = {};

			rj_writer writer;
			rj_file_writer file_writer;
			rj_buffer_writer buffer_writer;
			rj_streambuf_writer streambuf_writer;
		};
	};

	typedef basic_output_archive<pretty_print | inline_arrays | extended_fp> output_archive;

	static_assert(serialization::output_archive<output_archive, std::nullptr_t>);
	static_assert(serialization::output_archive<output_archive, bool>);
	static_assert(serialization::output_archive<output_archive, char>);
	static_assert(serialization::output_archive<output_archive, std::uint8_t>);
	static_assert(serialization::output_archive<output_archive, std::int8_t>);
	static_assert(serialization::output_archive<output_archive, std::int16_t>);
	static_assert(serialization::output_archive<output_archive, std::int32_t>);
	static_assert(serialization::output_archive<output_archive, std::int64_t>);
	static_assert(serialization::output_archive<output_archive, float>);
	static_assert(serialization::output_archive<output_archive, double>);
	static_assert(serialization::output_archive<output_archive, std::string>);
}	 // namespace sek::serialization::json