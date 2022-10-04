//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include <locale>

#include "any.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T>
		constexpr string_type_data string_type_data::make_instance() noexcept
		{
			// clang-format off
			using char_t = std::ranges::range_value_t<T>;
			using traits_t = std::conditional_t<requires { typename T::traits_type; }, typename T::traits_type, std::char_traits<char_t>>;
			// clang-format on

			string_type_data result;

			result.char_type = type_handle{type_selector<char_t>};
			result.traits_type = type_handle{type_selector<traits_t>};
			result.empty = +[](const void *p) -> bool { return std::ranges::empty(*static_cast<const T *>(p)); };
			result.size = +[](const void *p) -> std::size_t
			{
				const auto &obj = *static_cast<const T *>(p);
				return static_cast<std::size_t>(std::ranges::size(obj));
			};
			result.data = +[](any_ref &target) -> void *
			{
				if (target.is_const()) [[unlikely]]
					return nullptr;

				/* Views do not allow for non-const access. */
				auto &str = *static_cast<T *>(target.data());
				if constexpr (!requires { str.data(); })
				{
					const auto iter = std::ranges::begin(str);
					if constexpr (std::is_convertible_v<decltype(std::to_address(iter)), void *>)
						return std::to_address(iter);
				}
				else if constexpr (std::is_convertible_v<decltype(str.data()), void *>)
					return str.data();
				return nullptr;
			};
			result.cdata = +[](const any_ref &target) -> const void *
			{
				auto &str = *static_cast<const T *>(target.data());
				if constexpr (!requires { str.data(); })
					return std::to_address(std::ranges::begin(str));
				else
					return str.data();
			};

			return result;
		}
		template<typename T>
		constinit const string_type_data string_type_data::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Proxy structure used to operate on a string-like type-erased object. */
	class SEK_API any_string
	{
		friend class any;
		friend class any_ref;

	public:
		typedef std::size_t size_type;

	private:
		// clang-format off
		template<typename Sc, typename Dc, typename Dt, typename Da, typename State>
		static void do_convert(const Sc *src_start, const Sc *src_end, std::basic_string<Dc, Dt, Da> &dst,
							   const std::codecvt<Dc, Sc, State> &conv)
		// clang-format on
		{
			using conv_t = std::codecvt<Sc, Dt, State>;

			const auto src_length = src_end - src_start;
			const auto conv_max = conv.max_length();
			dst.assign(src_length * conv_max, '\0');

			const auto *src_pos = src_start;
			auto *dst_end = dst.data() + dst.size();
			auto *dst_pos = dst.data();
			State state;
			for (;;)
			{
				// clang-format off
				auto res = conv.in(state,
								src_pos, src_end, src_pos,
								dst_pos, dst_end, dst_pos);
				// clang-format on
				if (res == conv_t::partial)
				{
					/* If there is not enough space, expand the buffer. */
					const auto dst_off = dst_pos - dst.data();
					const auto src_left = src_end - src_pos;

					dst.append(src_left * conv_max, '\0');
					dst_end = dst.data() + dst.size();
					dst_pos = dst.data() + dst_off;
					continue;
				}
				else if (res == conv_t::noconv) /* If no conversion is needed, use memcpy. */
					memcpy(dst_pos, src_start, src_length * sizeof(Sc));
				break;
			}

			/* Trim buffer end. */
			const auto dst_off = dst_pos - dst.data();
			dst.erase(dst_off, dst.size() - dst_off);
		}
		// clang-format off
		template<typename Sc, typename Dc, typename Dt, typename Da, typename State>
		static void do_convert(const Sc *src_start, const Sc *src_end, std::basic_string<Dc, Dt, Da> &dst,
							   const std::codecvt<Sc, Dc, State> &conv)
		// clang-format on
		{
			using conv_t = std::codecvt<Sc, Dt, State>;

			const auto src_length = src_end - src_start;
			const auto conv_max = conv.max_length();
			dst.assign(src_length * conv_max, '\0');

			const auto *src_pos = src_start;
			auto *dst_end = dst.data() + dst.size();
			auto *dst_pos = dst.data();
			State state;
			for (;;)
			{
				// clang-format off
				auto res = conv.out(state,
								src_pos, src_end, src_pos,
								dst_pos, dst_end, dst_pos);
				// clang-format on
				if (res == conv_t::partial)
				{
					/* If there is not enough space, expand the buffer. */
					const auto dst_off = dst_pos - dst.data();
					const auto src_left = src_end - src_pos;

					dst.append(src_left * conv_max, '\0');
					dst_end = dst.data() + dst.size();
					dst_pos = dst.data() + dst_off;
					continue;
				}
				else if (res == conv_t::noconv) /* If no conversion is needed, use memcpy. */
					memcpy(dst_pos, src_start, src_length * sizeof(Sc));
				break;
			}

			/* Trim buffer end. */
			const auto dst_off = dst_pos - dst.data();
			dst.erase(dst_off, dst.size() - dst_off);
		}

		static const detail::string_type_data *assert_data(const detail::type_data *data);

		any_string(std::in_place_t, const any_ref &ref) : m_data(ref.m_type->string_data), m_target(ref) {}
		any_string(std::in_place_t, any_ref &&ref) : m_data(ref.m_type->string_data), m_target(std::move(ref)) {}

	public:
		any_string() = delete;
		any_string(const any_string &) = delete;
		any_string &operator=(const any_string &) = delete;

		constexpr any_string(any_string &&other) noexcept : m_data(other.m_data), m_target(std::move(other.m_target)) {}
		constexpr any_string &operator=(any_string &&other) noexcept
		{
			m_data = other.m_data;
			m_target = std::move(other.m_target);
			return *this;
		}

		/** Initializes an `any_string` instance for an `any_ref` object.
		 * @param ref `any_ref` referencing a table object.
		 * @throw type_error If the referenced object is not a table. */
		explicit any_string(const any_ref &ref) : m_data(assert_data(ref.m_type)), m_target(ref) {}
		/** @copydoc any_string */
		explicit any_string(any_ref &&ref) : m_data(assert_data(ref.m_type)), m_target(std::move(ref)) {}

		/** Returns `any_ref` reference ot the target string. */
		[[nodiscard]] any_ref target() const noexcept { return m_target; }

		/** Returns the character type of the string. */
		[[nodiscard]] constexpr type_info char_type() const noexcept;
		/** Returns the value (character) type of the string. */
		[[nodiscard]] constexpr type_info value_type() const noexcept;
		/** Returns the character traits type of the string. */
		[[nodiscard]] constexpr type_info traits_type() const noexcept;

		/** Checks if the referenced string is empty. */
		[[nodiscard]] bool empty() const;
		/** Returns size of the referenced string. */
		[[nodiscard]] size_type size() const;

		/** Returns raw pointer to the data of the string. If the string is const, returns `nullptr`.*/
		[[nodiscard]] void *data();
		/** Returns raw pointer to the data of the string. */
		[[nodiscard]] const void *data() const;
		/** @copydoc data */
		[[nodiscard]] const void *cdata() const { return data(); }

		/** Returns pointer to the data of the string. If the requested character type is not
		 * the same as the actual character type, or the string is const, returns `nullptr`.*/
		template<typename C = char>
		[[nodiscard]] C *chars();
		/** Returns pointer to the data of the string. If the requested character type is not
		 * the same as the actual character type, returns `nullptr`.*/
		template<typename C = char>
		[[nodiscard]] const C *chars() const;

		/** Converts the referenced string to an `std::basic_string` for the specified encoding.
		 * @param loc Locale to use for encoding conversion.
		 * @param alloc Allocator to use for the result string.
		 * @return Referenced string, converted to the specified encoding, or `type_errc::INVALID_TYPE`
		 * if the string cannot be converted to the specified encoding.
		 * @note If the referenced string and `C` are of the standard character types (`char`, `wchar_t`, `char8_t`,
		 * `char16_t` or `char32_t`) for which a conversion using `std::codecvt` is available, uses the provided
		 * locale to preform string conversion. If a conversion error is encountered, incomplete result is returned. */
		template<typename C = char, typename T = std::char_traits<C>, typename A = std::allocator<T>>
		[[nodiscard]] auto as_str(std::nothrow_t, const std::locale &loc = {}, const A &alloc = A{}) const
			-> expected<std::basic_string<C, T, A>, std::error_code>;
		/** Converts the referenced string to an `std::basic_string` for the specified encoding.
		 * @param loc Locale to use for encoding conversion.
		 * @param alloc Allocator to use for the result string.
		 * @return Referenced string, converted to the specified encoding.
		 * @throw type_error If the string cannot be converted to the specified encoding.
		 * @note If the referenced string and `C` are of the standard character types (`char`, `wchar_t`, `char8_t`,
		 * `char16_t` or `char32_t`) for which a conversion using `std::codecvt` is available, uses the provided
		 * locale to preform string conversion. If a conversion error is encountered, incomplete result is returned. */
		template<typename C = char, typename T = std::char_traits<C>, typename A = std::allocator<T>>
		[[nodiscard]] std::basic_string<C, T, A> as_str(const std::locale &loc = {}, const A &alloc = A{}) const;

		constexpr void swap(any_string &other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_target, other.m_target);
		}
		friend constexpr void swap(any_string &a, any_string &b) noexcept { a.swap(b); }

	private:
		template<typename Sc, typename C, typename T, typename A>
		bool convert_with(std::basic_string<C, T, A> &dst, const std::locale &l, const A &a) const;

		const detail::string_type_data *m_data;
		any_ref m_target;
	};
}	 // namespace sek