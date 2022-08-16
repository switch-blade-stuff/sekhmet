/*
 * Created by switchblade on 2022-04-19
 */

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

		using base_archive = json_archive_base<char>;
		using rj_encoding = rapidjson::UTF8<>;

		struct rj_allocator : detail::buffer_allocator<rj_allocator_page_size>
		{
			using base_pool = detail::buffer_allocator<rj_allocator_page_size>;

			SEK_FORCE_INLINE constexpr static void Free(void *) {}

			constexpr static bool kNeedFree = false;

			constexpr rj_allocator() noexcept = default;
			constexpr explicit rj_allocator(std::pmr::memory_resource *res) noexcept : buffer_allocator(res) {}

			SEK_FORCE_INLINE void *Malloc(std::size_t n) { return base_pool::allocate(n); }
			SEK_FORCE_INLINE void *Realloc(void *old, std::size_t old_n, std::size_t n) noexcept
			{
				return base_pool::reallocate(old, old_n, n);
			}
		};
	}	 // namespace detail

	typedef int config_flags;
	constexpr config_flags no_flags = 0;

	/** Enables parsing single & multi-line comments in Json input. Enabled by default. */
	constexpr config_flags allow_comments = 1;
	/** Enables parsing trailing commas in Json input. */
	constexpr config_flags trailing_commas = 2;
	/** Enables non-standard floating-point values (NaN, inf). */
	constexpr config_flags extended_fp = 16;

	/** @details Archive used to read Json data. Internally uses the RapidJSON library.
	 *
	 * The archive itself does not do any deserialization, instead deserialization is done by special archive frames,
	 * which represent a Json object or array. These frames are then passed to deserialization functions
	 * of serializable types.
	 *
	 * @tparam Config Configuration flags used for the archive.
	 * @note Json input archives can outlive the source they were initialized from, and can thus be used
	 * to cache json data to be deserialized later. */
	template<config_flags Config>
	class basic_input_archive : detail::base_archive
	{
		using base_t = detail::base_archive;

	public:
		typedef typename base_t::tree_type tree_type;
		typedef typename base_t::read_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;
		typedef typename archive_frame::char_type char_type;
		typedef typename archive_frame::size_type size_type;
		typedef archive_reader<char_type> reader_type;

	private:
		using rj_parser = rapidjson::GenericReader<detail::rj_encoding, detail::rj_encoding, detail::rj_allocator>;

		struct rj_reader : reader_type
		{
			using Ch = char_type;
			using base_t = reader_type;

			constexpr explicit rj_reader(base_t &&reader) : base_t(std::move(reader)) {}

			constexpr char_type Peek()
			{
				const auto result = base_t::peek();
				return base_t::traits_type::not_eof(result) ? base_t::traits_type::to_char_type(result) : '\0';
			}
			constexpr char_type Take()
			{
				const auto result = base_t::take();
				return base_t::traits_type::not_eof(result) ? base_t::traits_type::to_char_type(result) : '\0';
			}
			constexpr size_type Tell() { return base_t::tell(); }

			[[noreturn]] void Put(char_type) { SEK_NEVER_REACHED; }
			[[noreturn]] void Flush() { SEK_NEVER_REACHED; }
			[[noreturn]] char_type *PutBegin() { SEK_NEVER_REACHED; }
			[[noreturn]] size_type PutEnd(char_type *) { SEK_NEVER_REACHED; }
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
		basic_input_archive() = delete;
		basic_input_archive(const basic_input_archive &) = delete;
		basic_input_archive &operator=(const basic_input_archive &) = delete;

		constexpr basic_input_archive(basic_input_archive &&) noexcept = default;
		constexpr basic_input_archive &operator=(basic_input_archive &&other) noexcept
		{
			base_t::operator=(std::forward<basic_input_archive>(other));
			return *this;
		}

		/** Initializes input archive from a json node tree.
		 * @param tree Json node tree containing source data. */
		explicit basic_input_archive(tree_type &tree) : basic_input_archive(tree, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive */
		explicit basic_input_archive(tree_type &&tree)
			: basic_input_archive(std::move(tree), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(tree_type &tree, std::pmr::memory_resource *res) : base_t(tree, res) {}
		/** @copydoc basic_input_archive */
		basic_input_archive(tree_type &&tree, std::pmr::memory_resource *res) : base_t(std::move(tree), res) {}

		/** Reads Json using the provided archive reader.
		 * @param reader Reader used to read Json data. */
		explicit basic_input_archive(reader_type reader)
			: basic_input_archive(std::move(reader), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(reader_type reader, std::pmr::memory_resource *res) : base_t(res)
		{
			parse(rj_reader{std::move(reader)});
		}
		/** Reads Json from a character buffer.
		 * @param buff Pointer to the character buffer containing Json data.
		 * @param len Size of the character buffer. */
		basic_input_archive(const char_type *buff, std::size_t len)
			: basic_input_archive(buff, len, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res PMR memory resource used for internal allocation. */
		basic_input_archive(const char_type *buff, std::size_t len, std::pmr::memory_resource *res)
			: basic_input_archive(reader_type{buff, len}, res)
		{
		}
		/** Reads Json from a file.
		 * @param file Native file containing Json data. */
		explicit basic_input_archive(system::native_file &file)
			: basic_input_archive(file, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(system::native_file &file, std::pmr::memory_resource *res)
			: basic_input_archive(reader_type{file}, res)
		{
		}
		/** Reads Json from a file.
		 * @param file Pointer to the Json file. */
		explicit basic_input_archive(FILE *file) : basic_input_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(FILE *file, std::pmr::memory_resource *res) : basic_input_archive(reader_type{file}, res) {}
		/** Reads Json from a stream buffer.
		 * @param buff Pointer to the stream buffer. */
		explicit basic_input_archive(std::streambuf *buff) : basic_input_archive(buff, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(std::streambuf *buff, std::pmr::memory_resource *res)
			: basic_input_archive(reader_type{buff}, res)
		{
		}
		/** Reads Json from an input stream.
		 * @param is Reference to the input stream. */
		explicit basic_input_archive(std::istream &is) : basic_input_archive(is.rdbuf()) {}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(std::istream &is, std::pmr::memory_resource *res) : basic_input_archive(is.rdbuf(), res) {}

		/** Attempts to deserialize the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @param args Arguments forwarded to the deserialization function.
		 * @return true if deserialization was successful, false otherwise. */
		template<typename T, typename... Args>
		bool try_read(T &&value, Args &&...args)
		{
			return base_t::do_try_read(std::forward<T>(value), std::forward<Args>(args)...);
		}
		/** Deserializes the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return Reference to this archive.
		 * @throw archive_error On deserialization errors. */
		template<typename T>
		basic_input_archive &operator>>(T &&value)
		{
			return read(std::forward<T>(value));
		}
		/** @copydoc operator>>
		 * @param args Arguments forwarded to the deserialization function. */
		template<typename T, typename... Args>
		basic_input_archive &read(T &&value, Args &&...args)
		{
			base_t::do_read(std::forward<T>(value), std::forward<Args>(args)...);
			return *this;
		}
		/** @brief Deserializes an instance of `T` from the top-level Json entry of the archive in-place.
		 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
		 * or constructor accepting the archive frame as one of it's arguments if available.
		 * Otherwise, default-constructs & deserializes using `read(T &&)`.
		 * @param args Arguments forwarded to the deserialization function.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<typename T, typename... Args>
		T read(std::in_place_type_t<T>, Args &&...args)
		{
			return base_t::do_read(std::in_place_type<T>, std::forward<Args>(args)...);
		}

		constexpr void swap(basic_input_archive &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(basic_input_archive &a, basic_input_archive &b) noexcept { a.swap(b); }

		using base_t::tree;

	private:
		void parse(rj_reader reader)
		{
			rj_event_handler handler{*this};
			detail::rj_allocator allocator{base_t::upstream};
			rj_parser parser{&allocator};

			constexpr unsigned parse_flags =
				((Config & allow_comments) == allow_comments ? rapidjson::kParseCommentsFlag : 0) |
				((Config & trailing_commas) == trailing_commas ? rapidjson::kParseTrailingCommasFlag : 0) |
				((Config & extended_fp) == extended_fp ? rapidjson::kParseNanAndInfFlag : 0);
			if (!parser.Parse<parse_flags>(reader, handler)) [[unlikely]]
			{
				std::string error_msg = "Json parser error at ";
				error_msg.append(std::to_string(parser.GetErrorOffset()));
				throw archive_error(error_msg);
			}
		}
	};

	typedef basic_input_archive<allow_comments> input_archive;

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
	static_assert(serialization::structured_data_archive<input_archive::archive_frame>);

	/** Enables pretty-printing of Json output. Enabled by default. */
	constexpr config_flags pretty_print = 4;
	/** If pretty printing is enabled, writes arrays on a single line. Enabled by default. */
	constexpr config_flags inline_arrays = 8;

	/** @details Archive used to write Json data. Internally uses the RapidJSON library.
	 *
	 * The archive itself does not do any serialization, instead serialization is done by archive frames,
	 * which represent a Json object or array. These frames are then passed to serialization functions
	 * of serializable types.
	 *
	 * @tparam Config Configuration flags used for the archive.
	 * @tparam IndentC Indentation character used if `pretty_print` flag is present.
	 * @tparam IndentN Amount of indentation characters used if `pretty_print` flag is present.
	 * @note Json output archives may not outlive the destination stream, buffer, file or archive writer they were initialized from. */
	template<config_flags Config, char IndentC = ' ', std::size_t IndentN = 4>
	class basic_output_archive : detail::base_archive
	{
		using base_t = detail::base_archive;

	public:
		typedef typename base_t::tree_type tree_type;
		typedef typename base_t::write_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;
		typedef typename archive_frame::char_type char_type;
		typedef typename archive_frame::size_type size_type;
		typedef archive_writer<char_type> writer_type;

	private:
		struct rj_writer : writer_type
		{
			using Ch = char_type;
			using base_t = writer_type;

			constexpr explicit rj_writer(base_t &&writer) : base_t(std::move(writer)) {}

			void Put(char_type c) { base_t::put(c); }
			void Flush() { base_t::flush(); }
			size_type Tell() { return base_t::tell(); }

			[[noreturn]] char_type Peek() const { SEK_NEVER_REACHED; }
			[[noreturn]] char_type Take() { SEK_NEVER_REACHED; }
			[[noreturn]] char_type *PutBegin() { SEK_NEVER_REACHED; }
			[[noreturn]] size_type PutEnd(char_type *) { SEK_NEVER_REACHED; }
		};

		constexpr static auto rj_emitter_flags =
			rapidjson::kWriteDefaultFlags | ((Config & extended_fp) == extended_fp ? rapidjson::kWriteNanAndInfFlag : 0);
		using rj_emitter_base =
			std::conditional_t<(Config & pretty_print) == pretty_print,
							   rapidjson::PrettyWriter<rj_writer, detail::rj_encoding, detail::rj_encoding, detail::rj_allocator, rj_emitter_flags>,
							   rapidjson::Writer<rj_writer, detail::rj_encoding, detail::rj_encoding, detail::rj_allocator, rj_emitter_flags>>;

		struct rj_emitter : rj_emitter_base
		{
			struct frame_t
			{
				std::size_t size = 0;
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
			SEK_FORCE_INLINE void on_int(detail::json_type type, std::intmax_t value)
			{
				switch (type)
				{
					case detail::json_type::INT_S8:
					case detail::json_type::INT_S16:
					case detail::json_type::INT_S32: rj_emitter_base::Int(static_cast<int>(value)); break;
					case detail::json_type::INT_S64: rj_emitter_base::Int64(static_cast<std::int64_t>(value)); break;
					default: [[unlikely]] SEK_NEVER_REACHED;
				}
			}
			SEK_FORCE_INLINE void on_uint(detail::json_type type, std::uintmax_t value)
			{
				switch (type)
				{
					case detail::json_type::INT_U8:
					case detail::json_type::INT_U16:
					case detail::json_type::INT_U32: rj_emitter_base::Uint(static_cast<unsigned>(value)); break;
					case detail::json_type::INT_U64: rj_emitter_base::Uint64(static_cast<std::uint64_t>(value)); break;
					default: [[unlikely]] SEK_NEVER_REACHED;
				}
			}
			SEK_FORCE_INLINE void on_float32(float value) { on_float64(static_cast<double>(value)); }
			SEK_FORCE_INLINE void on_float64(double value) { rj_emitter_base::Double(value); }
			SEK_FORCE_INLINE void on_string(const char_type *str, std::size_t size)
			{
				rj_emitter_base::String(str, static_cast<unsigned>(size));
			}
			SEK_FORCE_INLINE void on_array_start(std::size_t size, auto)
			{
				frame.size = size;
				rj_emitter_base::StartArray();
			}
			SEK_FORCE_INLINE void on_array_end() { rj_emitter_base::EndArray(static_cast<unsigned>(frame.size)); }
			SEK_FORCE_INLINE void on_object_start(std::size_t size, auto)
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
			: base_t(std::forward<base_t>(other)), m_writer(std::move(other.m_writer))
		{
		}
		constexpr basic_output_archive &operator=(basic_output_archive &&other) noexcept
		{
			base_t::operator=(std::forward<base_t>(other));
			std::swap(m_writer, other.m_writer);
			return *this;
		}

		/** Initializes output archive from a json node tree.
		 * @param tree Json node tree containing source data. */
		explicit basic_output_archive(tree_type &tree) : basic_output_archive(tree, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive */
		explicit basic_output_archive(tree_type &&tree)
			: basic_output_archive(std::move(tree), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_output_archive(tree_type &tree, std::pmr::memory_resource *res) : base_t(tree, res), m_can_flush(false) {}
		/** @copydoc basic_input_archive */
		basic_output_archive(tree_type &&tree, std::pmr::memory_resource *res)
			: base_t(std::move(tree), res), m_can_flush(false)
		{
		}

		/** Initializes output archive for writing using the provided writer.
		 * @param writer Writer used to write Json data. */
		explicit basic_output_archive(writer_type writer)
			: basic_output_archive(std::move(writer), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_output_archive(writer_type writer, std::pmr::memory_resource *res)
			: base_t(res), m_writer(std::move(writer))
		{
		}
		/** Initializes output archive for writing to a string.
		 * @param str String to write Json data to. */
		template<typename Traits = std::char_traits<char_type>, typename Alloc = std::allocator<char_type>>
		explicit basic_output_archive(std::basic_string<char_type, Traits, Alloc> &str)
			: basic_output_archive(str, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		template<typename Traits = std::char_traits<char_type>, typename Alloc = std::allocator<char_type>>
		basic_output_archive(std::basic_string<char_type, Traits, Alloc> &str, std::pmr::memory_resource *res)
			: basic_output_archive(writer_type{str}, res)
		{
		}
		/** Initializes output archive for file writing.
		 * @param file Native file to write Json data to. */
		explicit basic_output_archive(system::native_file &file)
			: basic_output_archive(file, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_output_archive(system::native_file &file, std::pmr::memory_resource *res)
			: basic_output_archive(writer_type{file}, res)
		{
		}
		/** Initializes output archive for file writing.
		 * @param file File to write Json data to. */
		explicit basic_output_archive(FILE *file) : basic_output_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(FILE *file, std::pmr::memory_resource *res) : basic_output_archive(writer_type{file}, res)
		{
		}
		/** Initializes output archive for stream buffer writing.
		 * @param buff Stream buffer to write Json data to. */
		explicit basic_output_archive(std::streambuf *buff)
			: basic_output_archive(buff, std::pmr::get_default_resource())
		{
		}
		/** @copydoc output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(std::streambuf *buff, std::pmr::memory_resource *res)
			: basic_output_archive(writer_type{buff}, res)
		{
		}
		/** Initializes output archive for stream writing.
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
		basic_output_archive &operator<<(T &&value)
		{
			return write(std::forward<T>(value));
		}
		/** @copydoc operator<<
		 * @param args Arguments forwarded to the serialization function. */
		template<typename T, typename... Args>
		basic_output_archive &write(T &&value, Args &&...args)
		{
			/* Flush uncommitted changes before initializing a new emit tree. */
			flush();
			base_t::do_write(std::forward<T>(value), std::forward<Args>(args)...);
			return *this;
		}
		/** Flushes the internal state & writes Json to the output. */
		void flush()
		{
			flush_impl();
			base_t::reset();
		}

		/** Replaces the internal node tree with the specified one and returns pointer to the old tree. */
		constexpr tree_type *reset(tree_type *new_tree = nullptr) noexcept
		{
			m_can_flush = m_can_flush || new_tree != nullptr;
			return std::exchange(base_t::tree, new_tree);
		}

		constexpr void swap(basic_output_archive &other) noexcept
		{
			base_t::swap(other);
			std::swap(m_writer, other.m_writer);
		}
		friend constexpr void swap(basic_output_archive &a, basic_output_archive &b) noexcept { a.swap(b); }

	private:
		void flush_impl()
		{
			if (m_can_flush) [[likely]]
			{
				detail::rj_allocator allocator{base_t::upstream};
				rj_emitter emitter{m_writer, allocator};
				base_t::do_flush(emitter);
			}
		}

		union
		{
			std::byte m_padding[sizeof(rj_writer)] = {};
			rj_writer m_writer;
		};

		bool m_can_flush = true;
	};

	typedef basic_output_archive<pretty_print | inline_arrays> output_archive;

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