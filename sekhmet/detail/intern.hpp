/*
 * Created by switchblade on 10/05/22
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <bit>
#include <ranges>
#include <string>

#include "contiguous_iterator.hpp"
#include "dense_set.hpp"
#include "string_util.hpp"
#include <memory_resource>

namespace sek
{
	template<typename C, typename Traits = std::char_traits<C>>
	class basic_interned_string;
	template<typename C, typename Traits = std::char_traits<C>>
	class basic_intern_pool;

	namespace detail
	{
		template<typename C, typename T>
		struct intern_str_header
		{
			using parent_t = basic_intern_pool<C, T>;

			static intern_str_header *make_header(parent_t *p, const C *s, std::size_t n)
			{
				auto h = ::operator new(sizeof(intern_str_header) + (n + 1) * sizeof(C));
				return std::construct_at(static_cast<intern_str_header *>(h), p, s, n);
			}

			constexpr intern_str_header() noexcept = default;
			constexpr intern_str_header(parent_t *parent, const C *str, std::size_t n) noexcept
				: parent(parent), length(n)
			{
				*std::copy_n(str, n, data()) = '\0';
			}
			~intern_str_header();

			[[nodiscard]] constexpr C *data() noexcept
			{
				return std::bit_cast<C *>(std::bit_cast<std::byte *>(this) + sizeof(intern_str_header));
			}
			[[nodiscard]] constexpr const C *data() const noexcept
			{
				return std::bit_cast<const C *>(std::bit_cast<const std::byte *>(this) + sizeof(intern_str_header));
			}
			[[nodiscard]] constexpr std::basic_string_view<C, T> sv() const noexcept { return {data(), length}; }

			constexpr void acquire() noexcept { ref_count.fetch_add(1); }
			void release() noexcept
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
				{
					std::destroy_at(this);
					::operator delete(this);
				}
			}

			/* Reference count of the interned string. */
			std::atomic<std::size_t> ref_count = 0;
			/* Pointer to the pool that manages this string. */
			parent_t *parent;
			/* Length (in characters) of this string. */
			std::size_t length;
			/* String data follows the header. */
		};
	}	 // namespace detail

	/** @brief String-view like container used to intern strings via a global pool.
	 *
	 * Internally, all interned strings act as reference-counted pointers to implementation-defined
	 * structures allocated by the intern pool. Values of interned strings stay allocated as long as there
	 * are any references to them.
	 *
	 * @tparam C Character type of the interned string.
	 * @tparam Traits Character traits of `C`. */
	template<typename C, typename Traits>
	class basic_interned_string
	{
	public:
		using pool_type = basic_intern_pool<C, Traits>;

		typedef Traits traits_type;
		typedef C value_type;
		typedef const value_type *pointer;
		typedef const value_type *const_pointer;
		typedef const value_type &reference;
		typedef const value_type &const_reference;
		typedef contiguous_iterator<value_type, true> iterator;
		typedef contiguous_iterator<value_type, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr static size_type npos = std::numeric_limits<size_type>::max();

	private:
		template<typename, typename>
		friend class basic_intern_pool;

		using header_t = detail::intern_str_header<C, Traits>;

		constexpr explicit basic_interned_string(header_t *h) : m_header(h), m_length(h ? h->length : 0) { acquire(); }
		constexpr basic_interned_string(std::in_place_t, header_t *h) : m_header(h), m_length(h->length) {}

	public:
		/** Initializes an empty string. */
		constexpr basic_interned_string() noexcept = default;

		constexpr basic_interned_string(const basic_interned_string &other) noexcept
			: basic_interned_string(other.m_header)
		{
		}
		basic_interned_string &operator=(const basic_interned_string &other)
		{
			release(other.m_header);
			acquire();
			return *this;
		}
		constexpr basic_interned_string(basic_interned_string &&other) noexcept
			: m_header(std::exchange(other.m_header, nullptr)), m_length(m_header->length)
		{
		}
		constexpr basic_interned_string &operator=(basic_interned_string &&other) noexcept
		{
			swap(other);
			return *this;
		}
		constexpr ~basic_interned_string() { release(); }

		// clang-format off
		/** Interns the passed string using the provided pool. */
		template<typename R>
		basic_interned_string(pool_type &pool, const R &r) requires(std::ranges::forward_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, C>);
		/** Interns the passed string using the global (per-thread) pool.
		  * @note Global pools are thread-specific to avoid the need for synchronization. */
		template<typename R>
		basic_interned_string(const R &r) requires(std::ranges::forward_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, C>);
		// clang-format on

		/** @copydoc basic_interned_string */
		basic_interned_string(pool_type &pool, std::basic_string_view<C, Traits> sv);
		/** @copydoc basic_interned_string */
		basic_interned_string(pool_type &pool, const C *str);
		/** @copydoc basic_interned_string */
		basic_interned_string(pool_type &pool, const C *str, size_type n);

		/** @copydoc basic_interned_string */
		basic_interned_string(std::basic_string_view<C, Traits> sv);
		/** @copydoc basic_interned_string */
		basic_interned_string(const C *str);
		/** @copydoc basic_interned_string */
		basic_interned_string(const C *str, size_type n);

		/** Returns iterator to the start of the string. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return iterator{data()}; }
		/** Returns iterator to the end of the string. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return iterator{data() + size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the end of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
		/** Returns reverse iterator to the start of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns pointer to the string's data. */
		[[nodiscard]] constexpr const_pointer data() const noexcept
		{
			if (m_header) [[likely]]
				return m_header->data();
			else
				return nullptr;
		}
		/** @copydoc data */
		[[nodiscard]] constexpr const_pointer c_str() const noexcept { return data(); }
		/** Returns reference to the element at the specified offset within the string. */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return data()[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }
		/** Returns reference to the element at the start of the string. */
		[[nodiscard]] constexpr const_reference front() const noexcept { return data()[0]; }
		/** Returns reference to the element at the end of the string. */
		[[nodiscard]] constexpr const_reference back() const noexcept { return data()[size() - 1]; }

		/** Returns size of the string. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_length; }
		/** @copydoc size */
		[[nodiscard]] constexpr size_type length() const noexcept { return size(); }
		/** Returns maximum value for size. */
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return std::numeric_limits<size_type>::max() - 1;
		}
		/** Checks if the string is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Returns a string copy of this interned string. */
		template<typename Alloc = std::allocator<C>>
		[[nodiscard]] constexpr std::basic_string<C, Traits, Alloc> str() const noexcept
		{
			if (m_header) [[likely]]
				return std::basic_string<C, Traits, Alloc>{data(), size()};
			else
				return {};
		}
		/** @copydoc str */
		template<typename Alloc = std::allocator<C>>
		[[nodiscard]] constexpr operator std::basic_string<C, Traits, Alloc>() const noexcept
		{
			return str<Alloc>();
		}
		/** Converts the interned string to a string view. */
		[[nodiscard]] constexpr std::basic_string_view<C, Traits> sv() const noexcept
		{
			if (m_header) [[likely]]
				return m_header->sv();
			else
				return {};
		}
		/** @copydoc sv */
		[[nodiscard]] constexpr operator std::basic_string_view<C, Traits>() const noexcept { return sv(); }

		/** Finds left-most location of a sequence of character within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_first(Iterator first, Iterator last) const
		{
			return detail::find_first<npos>(begin(), end(), first, last);
		}
		/** Finds left-most location of a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_first(const value_type *str, size_type len) const noexcept
		{
			return find_first(str, str + len);
		}
		/** Finds left-most location of a c-style substring within the string. */
		[[nodiscard]] constexpr size_type find_first(const value_type *str) const noexcept
		{
			return find_first(str, detail::str_length(str));
		}
		/** Finds left-most location of a character range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_first(const R &r) const
		{
			return find_first(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Finds left-most location of a character within the string. */
		[[nodiscard]] constexpr size_type find_first(value_type c) const noexcept
		{
			for (auto character = begin(), last = end(); character != last; character++)
				if (*character == c) return static_cast<size_type>(character - begin());
			return npos;
		}

		/** Finds right-most location of a sequence of character within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_last(Iterator first, Iterator last) const
		{
			return detail::find_last<npos>(begin(), end(), first, last);
		}
		/** Finds right-most location of a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_last(const value_type *str, size_type len) const noexcept
		{
			return find_last(str, str + len);
		}
		/** Finds right-most location of a c-style substring within the string. */
		[[nodiscard]] constexpr size_type find_last(const value_type *str) const noexcept
		{
			return find_last(str, detail::str_length(str));
		}
		/** Finds right-most location of a character range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_last(const R &r) const
		{
			return find_last(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Finds right-most location of a character within the string. */
		[[nodiscard]] constexpr size_type find_last(value_type c) const noexcept
		{
			for (auto first = begin(), character = end(); character != first;)
				if (*(--character) == c) return static_cast<size_type>(character - begin());
			return npos;
		}

		/** Finds left-most location of a character from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_first_of(Iterator first, Iterator last) const
		{
			return detail::find_first_of<npos>(begin(), end(), first, last);
		}
		/** Finds left-most location of a character from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_first_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_first_of(list.begin(), list.end());
		}
		/** Finds left-most location of a character from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_first_of(const value_type *str, size_type len) const noexcept
		{
			return find_first_of(str, str + len);
		}
		/** Finds left-most location of a character from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_first_of(const value_type *str) const noexcept
		{
			return find_first_of(str, detail::str_length(str));
		}
		/** Finds left-most location of a character from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_first_of(const R &r) const
		{
			return find_first_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Finds right-most location of a character from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_last_of(Iterator first, Iterator last) const
		{
			return detail::find_last_of<npos>(begin(), end(), first, last);
		}
		/** Finds right-most location of a character from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_last_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_last_of(list.begin(), list.end());
		}
		/** Finds right-most location of a character from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_last_of(const value_type *str, size_type len) const noexcept
		{
			return find_last_of(str, str + len);
		}
		/** Finds right-most location of a character from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_last_of(const value_type *str) const noexcept
		{
			return find_last_of(str, detail::str_length(str));
		}
		/** Finds right-most location of a character from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_last_of(const R &r) const
		{
			return find_last_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Finds left-most location of a character not from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_first_not_of(Iterator first, Iterator last) const
		{
			return detail::find_first_not_of<npos>(begin(), end(), first, last);
		}
		/** Finds left-most location of a character not from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_first_not_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_first_not_of(list.begin(), list.end());
		}
		/** Finds left-most location of a character not from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_first_not_of(const value_type *str) const noexcept
		{
			return find_first_not_of(str, detail::str_length(str));
		}
		/** Finds left-most location of a character not from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_first_not_of(const value_type *str, size_type len) const noexcept
		{
			return find_first_not_of(str, str + len);
		}
		/** Finds left-most location of a character not from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_first_not_of(const R &r) const
		{
			return find_first_not_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Finds right-most location of a character not from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_last_not_of(Iterator first, Iterator last) const
		{
			return detail::find_last_not_of<npos>(begin(), end(), first, last);
		}
		/** Finds right-most location of a character not from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_last_not_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_last_not_of(list.begin(), list.end());
		}
		/** Finds right-most location of a character not from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_last_not_of(const value_type *str) const noexcept
		{
			return find_last_not_of(str, detail::str_length(str));
		}
		/** Finds right-most location of a character not from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_last_not_of(const value_type *str, size_type len) const noexcept
		{
			return find_last_not_of(str, str + len);
		}
		/** Finds right-most location of a character not from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_last_not_of(const R &r) const
		{
			return find_last_not_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Checks if a substring is present within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr bool contains(Iterator first, Iterator last) const
		{
			return find_first(first, last) != npos;
		}
		/** Checks if a substring is present within the string. */
		[[nodiscard]] constexpr bool contains(const value_type *str, size_type len) const noexcept
		{
			return contains(str, str + len);
		}
		/** Checks if a substring is present within the string. */
		[[nodiscard]] constexpr bool contains(const value_type *str) const noexcept
		{
			return contains(str, detail::str_length(str));
		}
		/** Checks if a range of characters is present within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr bool contains(const R &r) const
		{
			return contains(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Checks if a character is present within the string. */
		[[nodiscard]] constexpr bool contains(value_type c) const noexcept { return find_first(c) != npos; }

		/** Checks if a substring is located at the start of the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr bool has_prefix(Iterator first, Iterator last) const
		{
			return detail::has_prefix(begin(), end(), first, last);
		}
		/** Checks if a substring is located at the start of the string. */
		[[nodiscard]] constexpr bool has_prefix(const value_type *str) const noexcept
		{
			return has_prefix(str, detail::str_length(str));
		}
		/** Checks if a substring is located at the start of the string. */
		[[nodiscard]] constexpr bool has_prefix(const value_type *str, size_type len) const noexcept
		{
			return has_prefix(str, str + len);
		}
		/** Checks if a range of characters is located at the start of the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr bool has_prefix(const R &r) const
		{
			return has_prefix(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Checks if a character is located at the start of the string. */
		[[nodiscard]] constexpr bool has_prefix(value_type c) const noexcept { return front() == c; }

		/** Checks if a substring is located at the end of the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr bool has_postfix(Iterator first, Iterator last) const
		{
			return detail::has_postfix(begin(), end(), first, last);
		}
		/** Checks if a substring is located at the end of the string. */
		[[nodiscard]] constexpr bool has_postfix(const value_type *str) const noexcept
		{
			return has_postfix(str, detail::str_length(str));
		}
		/** Checks if a substring is located at the end of the string. */
		[[nodiscard]] constexpr bool has_postfix(const value_type *str, size_type len) const noexcept
		{
			return has_postfix(str, str + len);
		}
		/** Checks if a range of characters is located at the end of the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr bool has_postfix(const R &r) const
		{
			return has_postfix(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Checks if a character is located at the end of the string. */
		[[nodiscard]] constexpr bool has_postfix(value_type c) const noexcept { return back() == c; }

		[[nodiscard]] friend constexpr auto operator<=>(const basic_interned_string &a, const basic_interned_string &b) noexcept
		{
			return a.sv() <=> b.sv();
		}
		[[nodiscard]] friend constexpr bool operator==(const basic_interned_string &a, const basic_interned_string &b) noexcept
		{
			return a.sv() == b.sv();
		}

		constexpr void swap(basic_interned_string &other) noexcept
		{
			std::swap(m_header, other.m_header);
			std::swap(m_length, other.m_length);
		}
		friend constexpr void swap(basic_interned_string &a, basic_interned_string &b) noexcept { a.swap(b); }

	private:
		constexpr void acquire()
		{
			if (m_header) [[likely]]
				m_header->acquire();
		}
		void release(header_t *new_header)
		{
			release();
			m_header = new_header;
			m_length = new_header->length;
		}
		void release()
		{
			if (m_header) [[likely]]
				m_header->release();
		}

		header_t *m_header = nullptr;
		std::size_t m_length = 0;
	};

	/** @brief Memory pool used to allocate & manage interned strings.
	 *
	 * @tparam C Character type of the strings allocated by the pool.
	 * @tparam Traits Character traits of `C`. */
	template<typename C, typename Traits>
	class basic_intern_pool
	{
	public:
		using string_type = basic_interned_string<C, Traits>;
		typedef string_type value_type;

	private:
		using header_t = detail::intern_str_header<C, Traits>;
		using sv_t = std::basic_string_view<C, Traits>;

		friend string_type;
		friend header_t;

		static basic_intern_pool &global()
		{
			thread_local basic_intern_pool instance;
			return instance;
		}

		struct intern_hash
		{
			typedef std::true_type is_transparent;

			constexpr hash_t operator()(const header_t *s) const noexcept { return operator()(s->sv()); }
			constexpr hash_t operator()(sv_t sv) const noexcept { return fnv1a(sv.data(), sv.size()); }
		};
		struct intern_cmp
		{
			typedef std::true_type is_transparent;

			constexpr bool operator()(const header_t *a, const header_t *b) const noexcept
			{
				return a->sv() == b->sv();
			}
			constexpr bool operator()(sv_t a, const header_t *b) const noexcept { return a == b->sv(); }
			constexpr bool operator()(const header_t *a, sv_t b) const noexcept { return a->sv() == b; }
		};

		using data_t = dense_set<header_t *, intern_hash, intern_cmp>;

	public:
		constexpr basic_intern_pool() = default;
		constexpr basic_intern_pool(const basic_intern_pool &) = default;
		constexpr basic_intern_pool &operator=(const basic_intern_pool &) = default;
		constexpr basic_intern_pool(basic_intern_pool &&) noexcept(std::is_nothrow_move_constructible_v<data_t>) = default;
		constexpr basic_intern_pool &operator=(basic_intern_pool &&) noexcept(std::is_nothrow_move_assignable_v<data_t>) = default;
		constexpr ~basic_intern_pool() = default;

		/** Initializes the pool using the provided memory resource. */
		constexpr explicit basic_intern_pool(std::pmr::memory_resource *alloc) : m_data(alloc) {}

		/** Interns the passed string view. */
		[[nodiscard]] string_type intern(sv_t str) { return string_type{*this, str}; }
		/** Interns the passed string. */
		[[nodiscard]] string_type intern(const C *str) { return string_type{*this, str}; }
		/** @copydoc intern */
		[[nodiscard]] string_type intern(const C *str, std::size_t n) { return string_type{*this, str, n}; }

	private:
		[[nodiscard]] header_t *intern_impl(sv_t sv);
		void unintern(header_t *h);

		// clang-format off
		template<typename R>
		[[nodiscard]] header_t *intern_impl(const R &r) requires(std::ranges::forward_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, C>)
		{
			if constexpr (std::contiguous_iterator<std::ranges::iterator_t<R>>)
				return intern_impl(sv_t{std::to_address(std::ranges::begin(r)), std::ranges::size(r)});
			else
			{
				const auto temp = std::string{std::ranges::begin(r), std::ranges::end(r)};
				return intern_impl(sv_t{temp});
			}
		}
		// clang-format on

		data_t m_data;
	};

	// clang-format off
	template<typename C, typename Traits>
	template<typename R>
	basic_interned_string<C, Traits>::basic_interned_string(pool_type &pool, const R &r)
		requires(std::ranges::forward_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, C>)
		: basic_interned_string(pool.intern_impl(r))
	{
	}
	template<typename C, typename Traits>
	template<typename R>
	basic_interned_string<C, Traits>::basic_interned_string(const R &r)
		requires(std::ranges::forward_range<R> && std::is_convertible_v<std::ranges::range_value_t<R>, C>)
		: basic_interned_string(pool_type::global(), r)
	{
	}
	// clang-format on

	template<typename C, typename Traits>
	basic_interned_string<C, Traits>::basic_interned_string(pool_type &pool, std::basic_string_view<C, Traits> sv)
		: basic_interned_string(pool.intern_impl(sv))
	{
	}
	template<typename C, typename Traits>
	basic_interned_string<C, Traits>::basic_interned_string(pool_type &pool, const C *str)
		: basic_interned_string(pool, std::basic_string_view<C, Traits>{str})
	{
	}
	template<typename C, typename Traits>
	basic_interned_string<C, Traits>::basic_interned_string(pool_type &pool, const C *str, size_type n)
		: basic_interned_string(pool, std::basic_string_view<C, Traits>{str, n})
	{
	}
	template<typename C, typename Traits>
	basic_interned_string<C, Traits>::basic_interned_string(std::basic_string_view<C, Traits> sv)
		: basic_interned_string(pool_type::global(), sv)
	{
	}
	template<typename C, typename Traits>
	basic_interned_string<C, Traits>::basic_interned_string(const C *str)
		: basic_interned_string(std::basic_string_view<C, Traits>{str})
	{
	}
	template<typename C, typename Traits>
	basic_interned_string<C, Traits>::basic_interned_string(const C *str, size_type n)
		: basic_interned_string(std::basic_string_view<C, Traits>{str, n})
	{
	}

	template<typename C, typename Traits>
	typename basic_intern_pool<C, Traits>::header_t *basic_intern_pool<C, Traits>::intern_impl(sv_t sv)
	{
		if (sv.empty()) [[unlikely]]
			return nullptr;

		auto iter = m_data.find(sv);
		if (iter == m_data.end()) [[unlikely]]
			iter = m_data.emplace(header_t::make_header(this, sv.data(), sv.size())).first;
		return *iter;
	}
	template<typename C, typename Traits>
	void basic_intern_pool<C, Traits>::unintern(header_t *h)
	{
		m_data.erase(h->sv());
	}

	template<typename C, typename T>
	detail::intern_str_header<C, T>::~intern_str_header()
	{
		parent->unintern(this);
	}

	template<typename C, typename T>
	[[nodiscard]] constexpr hash_t hash(const basic_interned_string<C, T> &s) noexcept
	{
		return fnv1a(s.data(), s.size());
	}

	template<typename C, typename T, typename A>
	[[nodiscard]] constexpr auto operator<=>(const basic_interned_string<C, T> &a, const std::basic_string<C, T, A> &b) noexcept
	{
		return a.sv() <=> b;
	}
	template<typename C, typename T, typename A>
	[[nodiscard]] constexpr auto operator<=>(const std::basic_string<C, T, A> &a, const basic_interned_string<C, T> &b) noexcept
	{
		return a <=> b.sv();
	}
	template<typename C, typename T, typename A>
	[[nodiscard]] constexpr bool operator==(const basic_interned_string<C, T> &a, const std::basic_string<C, T, A> &b) noexcept
	{
		return a.sv() == b;
	}
	template<typename C, typename T, typename A>
	[[nodiscard]] constexpr bool operator==(const std::basic_string<C, T, A> &a, const basic_interned_string<C, T> &b) noexcept
	{
		return a == b.sv();
	}
	template<typename C, typename T>
	[[nodiscard]] constexpr auto operator<=>(const basic_interned_string<C, T> &a, const std::basic_string_view<C, T> &b) noexcept
	{
		return a.sv() <=> b;
	}
	template<typename C, typename T>
	[[nodiscard]] constexpr auto operator<=>(const std::basic_string_view<C, T> &a, const basic_interned_string<C, T> &b) noexcept
	{
		return a <=> b.sv();
	}
	template<typename C, typename T>
	[[nodiscard]] constexpr bool operator==(const basic_interned_string<C, T> &a, const std::basic_string_view<C, T> &b) noexcept
	{
		return a.sv() == b;
	}
	template<typename C, typename T>
	[[nodiscard]] constexpr bool operator==(const std::basic_string_view<C, T> &a, const basic_interned_string<C, T> &b) noexcept
	{
		return a == b.sv();
	}

	extern template SEK_API_IMPORT basic_intern_pool<char> &basic_intern_pool<char>::global();
	extern template SEK_API_IMPORT basic_intern_pool<wchar_t> &basic_intern_pool<wchar_t>::global();

	using intern_pool = basic_intern_pool<char>;
	using intern_wpool = basic_intern_pool<wchar_t>;
	using interned_string = basic_interned_string<char>;
	using interned_wstring = basic_interned_string<wchar_t>;
}	 // namespace sek

template<typename C, typename T>
struct std::hash<sek::basic_interned_string<C, T>>
{
	typedef std::true_type is_transparent;

	[[nodiscard]] constexpr sek::hash_t operator()(const sek::basic_interned_string<C, T> &s) const noexcept
	{
		return sek::fnv1a(s.data(), s.size());
	}
	[[nodiscard]] constexpr sek::hash_t operator()(std::basic_string_view<C, T> sv) const noexcept
	{
		return sek::fnv1a(sv.data(), sv.size());
	}
};
template<typename C, typename T>
struct std::equal_to<sek::basic_interned_string<C, T>>
{
	typedef std::true_type is_transparent;

	[[nodiscard]] constexpr bool operator()(const sek::basic_interned_string<C, T> &a,
											const sek::basic_interned_string<C, T> &b) const noexcept
	{
		return a == b;
	}
	template<typename K>
	[[nodiscard]] constexpr bool operator()(const sek::basic_interned_string<C, T> &a, K &&b) const noexcept
	{
		return a == b;
	}
	template<typename K>
	[[nodiscard]] constexpr bool operator()(K &&a, const sek::basic_interned_string<C, T> &b) const noexcept
	{
		return b == a;
	}
};