//
// Created by switchblade on 10/05/22.
//

#pragma once

#include <atomic>
#include <bit>
#include <string>

#include "contiguous_iterator.hpp"
#include "define.h"
#include "dense_hash_table.hpp"
#include "hash.hpp"
#include "string_util.hpp"

namespace sek
{
	namespace detail
	{
		template<typename C, typename T>
		struct intern_str_header
		{
			constexpr static intern_str_header *make_cb(const C *str, std::size_t n);

			constexpr intern_str_header(const C *str, std::size_t n) noexcept : sv(static_cast<const C *>(data()), n)
			{
				*std::copy_n(str, n, static_cast<C *>(data())) = '\0';
			}

			constexpr C *data() noexcept;

			constexpr void acquire() noexcept { ref_count.fetch_add(1); }
			constexpr void release() noexcept
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
					::operator delete(this);
			}

			/* Reference count of the interned string. */
			std::atomic<std::size_t> ref_count = 0;
			/* String view pointing to this header. */
			std::basic_string_view<C, T> sv;
			/* String data follows the header. */
		};

		template<typename C, typename T>
		constexpr intern_str_header<C, T> *intern_str_header<C, T>::make_cb(const C *str, std::size_t n)
		{
			auto cb = static_cast<intern_str_header *>(::operator new(sizeof(intern_str_header) + sizeof(C) * (n + 1)));
			return std::construct_at(cb, str, n);
		}
		template<typename C, typename T>
		constexpr C *intern_str_header<C, T>::data() noexcept
		{
			return std::bit_cast<C *>(std::bit_cast<std::byte *>(this) + sizeof(intern_str_header));
		}

		template<typename C, typename T>
		struct intern_str_ref
		{
			constexpr intern_str_ref() noexcept = default;

			constexpr explicit intern_str_ref(intern_str_header<C, T> *header) noexcept : header(header)
			{
				header->acquire();
			}

			constexpr intern_str_ref(const intern_str_ref &other) noexcept : intern_str_ref(other.header) {}
			constexpr intern_str_ref &operator=(const intern_str_ref &other)
			{
				release(other.header);
				acquire();
				return *this;
			}
			constexpr intern_str_ref(intern_str_ref &&other) noexcept : header(std::exchange(other.header, {})) {}
			constexpr intern_str_ref &operator=(intern_str_ref &&other) noexcept
			{
				std::swap(header, other.header);
				return *this;
			}
			constexpr ~intern_str_ref() { release(); }

			constexpr void acquire()
			{
				if (header) [[likely]]
					header->acquire();
			}
			constexpr void release()
			{
				if (header) [[likely]]
					header->release();
			}
			constexpr void release(intern_str_header<C, T> *new_header)
			{
				release();
				header = new_header;
			}

			constexpr void swap(intern_str_ref &other) noexcept { std::swap(header, other.header); }

			intern_str_header<C, T> *header;
		};
	}	 // namespace detail

	template<typename C, typename Traits = std::char_traits<C>>
	class basic_interned_string;

	/** @brief Memory pool used to allocate & manage interned strings.
	 *
	 * @tparam C Character type of the strings allocated by the pool.
	 * @tparam Traits Character traits of `C`. */
	template<typename C, typename Traits = std::char_traits<C>>
	class basic_intern_pool
	{
	public:
		using string_type = basic_interned_string<C, Traits>;
		using sv_type = std::basic_string_view<C, Traits>;

	private:
		friend string_type;

		using header_t = detail::intern_str_header<C, Traits>;

		class value_pointer;
		class value_reference;

		struct value_traits
		{
			template<bool>
			using iterator_value = string_type;
			template<bool>
			using iterator_reference = value_reference;
			template<bool>
			using iterator_pointer = value_pointer;
		};
		struct fnv_hash
		{
			constexpr hash_t operator()(const sv_type &s) const noexcept { return fnv1a(s.data(), s.size()); }
		};
		struct to_sv
		{
			constexpr const sv_type &operator()(const header_t *h) const noexcept { return h->sv; }
		};

		using data_t =
			detail::dense_hash_table<sv_type, header_t *, value_traits, fnv_hash, std::equal_to<>, to_sv, std::allocator<header_t *>>;

	public:
		typedef string_type value_type;
		typedef value_pointer pointer;
		typedef value_pointer const_pointer;
		typedef value_reference reference;
		typedef value_reference const_reference;
		typedef typename data_t::size_type size_type;
		typedef typename data_t::difference_type difference_type;

		typedef typename data_t::const_iterator iterator;
		typedef typename data_t::const_iterator const_iterator;
		typedef typename data_t::const_reverse_iterator reverse_iterator;
		typedef typename data_t::const_reverse_iterator const_reverse_iterator;

	private:
		class value_reference : detail::intern_str_ref<C, Traits>
		{
			friend data_t;
			friend iterator;
			friend string_type;
			friend class value_pointer;
			friend class basic_intern_pool;

			using ref_base = detail::intern_str_ref<C, Traits>;

			constexpr explicit value_reference(header_t *h) noexcept : ref_base(h) {}

		public:
			value_reference() = delete;

			constexpr value_reference(const value_reference &) noexcept = default;
			constexpr value_reference &operator=(const value_reference &) = default;
			constexpr value_reference(value_reference &&) noexcept = default;
			constexpr value_reference &operator=(value_reference &&) noexcept = default;
			constexpr ~value_reference() = default;

			constexpr string_type str() &&noexcept { return string_type{std::forward<ref_base>(*this)}; }
			constexpr operator string_type() &&noexcept { return str(); }
			constexpr string_type str() const &noexcept { return string_type{static_cast<const ref_base &>(*this)}; }
			constexpr operator string_type() const &noexcept { return str(); }

			[[nodiscard]] constexpr value_pointer operator&() &&noexcept
			{
				return value_pointer{std::forward<value_reference>(*this)};
			}
			[[nodiscard]] constexpr value_pointer operator&() const &noexcept { return value_pointer{*this}; }

			[[nodiscard]] constexpr operator bool() const noexcept { return ref_base::header != nullptr; }

			[[nodiscard]] constexpr auto operator<=>(const string_type &s) const noexcept
			{
				return sv_type{s} <=> ref_base::header->sv;
			}
			[[nodiscard]] constexpr auto operator<=>(sv_type sv) const noexcept { return sv <=> ref_base::header->sv; }
			[[nodiscard]] constexpr auto operator<=>(const value_reference &other) const noexcept
			{
				return operator<=>(to_sv{}(other.header));
			}

			[[nodiscard]] constexpr bool operator==(const string_type &s) const noexcept
			{
				return sv_type{s} == ref_base::header->sv;
			}
			[[nodiscard]] constexpr bool operator==(sv_type sv) const noexcept { return sv == ref_base::header->sv; }
			[[nodiscard]] constexpr bool operator==(const value_reference &other) const noexcept
			{
				return operator==(to_sv{}(other.header));
			}
		};
		class value_pointer
		{
			friend data_t;
			friend iterator;
			friend class value_reference;
			friend class basic_intern_pool;

			constexpr explicit value_pointer(header_t **h) noexcept : ref(*h) {}
			constexpr explicit value_pointer(value_reference &&ref) noexcept : ref(std::forward<value_reference>(ref))
			{
			}
			constexpr explicit value_pointer(const value_reference &ref) noexcept : ref(ref) {}

		public:
			value_pointer() = delete;

			constexpr value_pointer(const value_pointer &) noexcept = default;
			constexpr value_pointer &operator=(const value_pointer &) = default;
			constexpr value_pointer(value_pointer &&) noexcept = default;
			constexpr value_pointer &operator=(value_pointer &&) noexcept = default;
			constexpr ~value_pointer() = default;

			[[nodiscard]] constexpr const value_reference *get() const noexcept { return std::addressof(ref); }
			[[nodiscard]] constexpr const value_reference *operator->() const noexcept { return get(); }

			[[nodiscard]] constexpr value_reference operator*() &&noexcept { return std::move(ref); }
			[[nodiscard]] constexpr value_reference operator*() const &noexcept { return ref; }

			[[nodiscard]] constexpr operator bool() const noexcept { return ref; }

			[[nodiscard]] constexpr auto operator<=>(const value_pointer &other) const noexcept
			{
				return ref.header <=> other.ref.header;
			}
			[[nodiscard]] constexpr bool operator==(const value_pointer &other) const noexcept
			{
				return ref.header == other.ref.header;
			}

		private:
			value_reference ref;
		};

	public:
		/** Returns iterator to the first interned string within internal storage. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return data.cbegin(); }
		/** Returns iterator one past the last interned string within internal storage. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return data.cend(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the end of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return data.crbegin(); }
		/** Returns reverse iterator to the start of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return data.crend(); }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Interns the passed string view. */
		[[nodiscard]] constexpr string_type intern(sv_type str);
		/** Interns the passed string. */
		[[nodiscard]] constexpr string_type intern(const C *str);
		/** @copydoc intern */
		[[nodiscard]] constexpr string_type intern(const C *str, size_type n);

	private:
		[[nodiscard]] constexpr value_reference intern_impl(sv_type sv)
		{
			auto iter = data.find(sv);
			if (iter == data.end()) [[unlikely]]
				iter = data.insert(header_t::make_cb(sv.data(), sv.length())).first;
			return *iter;
		}

		data_t data;
	};

	/** @brief String-view like container used to intern strings via a global pool.
	 *
	 * Internally, all interned strings act as reference-counted pointers to implementation-defined
	 * structures allocated by the intern pool. Values of interned strings stay allocated as long as there
	 * are any references to them.
	 *
	 * @tparam C Character type of the interned string.
	 * @tparam Traits Character traits of `C`. */
	template<typename C, typename Traits>
	class basic_interned_string : detail::intern_str_ref<C, Traits>
	{
	public:
		typedef basic_intern_pool<C, Traits> pool_type;
		typedef std::basic_string_view<C, Traits> sv_type;

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
		friend pool_type;

		using ref_base = detail::intern_str_ref<C, Traits>;

		static pool_type &global_pool()
		{
			static pool_type instance;
			return instance;
		}

		constexpr explicit basic_interned_string(ref_base &&ref) : ref_base(std::forward<ref_base>(ref)) {}
		constexpr explicit basic_interned_string(const ref_base &ref) : ref_base(ref) {}
		constexpr explicit basic_interned_string(typename pool_type::reference &&ref)
			: ref_base(std::forward<ref_base>(ref))
		{
		}
		constexpr explicit basic_interned_string(const typename pool_type::reference &ref) : ref_base(ref) {}

	public:
		/** Initializes an empty string. */
		constexpr basic_interned_string() noexcept = default;

		/** Interns the passed string using the provided pool. */
		constexpr basic_interned_string(pool_type &pool, sv_type sv) : basic_interned_string(pool.intern_impl(sv)) {}
		/** @copydoc basic_interned_string */
		constexpr basic_interned_string(pool_type &pool, const C *str) : basic_interned_string(pool, sv_type{str}) {}
		/** @copydoc basic_interned_string */
		constexpr basic_interned_string(pool_type &pool, const C *str, size_type n)
			: basic_interned_string(pool, sv_type{str, n})
		{
		}

		/** Interns the passed string using the global pool. */
		constexpr basic_interned_string(sv_type sv) : basic_interned_string(global_pool(), sv) {}
		/** @copydoc basic_interned_string */
		constexpr basic_interned_string(const C *str) : basic_interned_string(sv_type{str}) {}
		/** @copydoc basic_interned_string */
		constexpr basic_interned_string(const C *str, size_type n) : basic_interned_string(sv_type{str, n}) {}

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
			if (ref_base::header) [[likely]]
				return ref_base::header->sv.data();
			else
				return 0;
		}
		/** Returns reference to the element at the specified offset within the string. */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return data()[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }
		/** Returns reference to the element at the start of the string. */
		[[nodiscard]] constexpr const_reference front() const noexcept { return data()[0]; }
		/** Returns reference to the element at the end of the string. */
		[[nodiscard]] constexpr const_reference back() const noexcept { return data()[size() - 1]; }

		/** Returns size of the string. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			if (ref_base::header) [[likely]]
				return ref_base::header->sv.size();
			else
				return 0;
		}
		/** @copydoc size */
		[[nodiscard]] constexpr size_type length() const noexcept { return size(); }
		/** Returns maximum value for size. */
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return std::numeric_limits<size_type>::max() - 1;
		}
		/** Checks if the string is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Converts the interned string to a string view. */
		[[nodiscard]] constexpr operator sv_type() const noexcept
		{
			if (ref_base::header) [[likely]]
				return ref_base::header->sv;
			else
				return {};
		}

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
			return sv_type{a} <=> sv_type{b};
		}
		[[nodiscard]] friend constexpr bool operator==(const basic_interned_string &a, const basic_interned_string &b) noexcept
		{
			return sv_type{a} == sv_type{b};
		}

		constexpr void swap(basic_interned_string &other) noexcept { ref_base::swap(other); }
		friend constexpr void swap(basic_interned_string &a, basic_interned_string &b) noexcept { a.swap(b); }
	};

	template<typename C, typename T>
	constexpr typename basic_intern_pool<C, T>::string_type basic_intern_pool<C, T>::intern(sv_type str)
	{
		return string_type{*this, str};
	}
	template<typename C, typename T>
	constexpr typename basic_intern_pool<C, T>::string_type basic_intern_pool<C, T>::intern(const C *str)
	{
		return string_type{*this, str};
	}
	template<typename C, typename T>
	constexpr typename basic_intern_pool<C, T>::string_type basic_intern_pool<C, T>::intern(const C *str, size_type n)
	{
		return string_type{*this, str, n};
	}

	template<typename C, typename T>
	[[nodiscard]] constexpr hash_t hash(const basic_interned_string<C, T> &s) noexcept
	{
		return fnv1a(s.data(), s.size());
	}

	extern template class SEK_API_IMPORT basic_intern_pool<char>;
	extern template class SEK_API_IMPORT basic_intern_pool<wchar_t>;
	extern template class SEK_API_IMPORT basic_interned_string<char>;
	extern template class SEK_API_IMPORT basic_interned_string<wchar_t>;

	using intern_pool = basic_intern_pool<char>;
	using intern_wpool = basic_intern_pool<wchar_t>;
	using interned_string = basic_interned_string<char>;
	using interned_wstring = basic_interned_string<wchar_t>;
}	 // namespace sek

template<typename C, typename T>
struct std::hash<sek::basic_interned_string<C, T>>
{
	[[nodiscard]] constexpr sek::hash_t operator()(const sek::basic_interned_string<C, T> &s) const noexcept
	{
		return sek::hash(s);
	}
};