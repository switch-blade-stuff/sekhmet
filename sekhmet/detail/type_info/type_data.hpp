//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include <array>
#include <memory>

#include "../../meta.hpp"
#include "../../type_name.hpp"
#include "traits.hpp"

namespace sek::detail
{
	/* Lazy handle to a `type_data`. */
	struct type_handle
	{
		constexpr type_handle() noexcept = default;
		template<typename T>
		constexpr explicit type_handle(type_selector_t<T>) noexcept;

		[[nodiscard]] constexpr type_data *operator->() const noexcept { return get(); }
		[[nodiscard]] constexpr type_data &operator*() const noexcept { return *get(); }

		[[nodiscard]] constexpr bool operator==(const type_handle &other) const noexcept;

		type_data *(*get)() noexcept = nullptr;
	};

	/* List used to store type data. */
	template<typename T>
	struct type_data_node
	{
		const T *next = nullptr;
	};
	template<typename T>
	struct type_data_list
	{
		struct list_iterator
		{
			typedef T value_type;
			typedef const T *pointer;
			typedef const T *const_pointer;
			typedef const T &reference;
			typedef const T &const_reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::size_t size_type;
			typedef std::forward_iterator_tag iterator_category;

			constexpr list_iterator() noexcept = default;
			constexpr list_iterator(pointer node) noexcept : node(node) {}

			constexpr list_iterator &operator++() noexcept
			{
				node = node->next;
				return *this;
			}
			constexpr list_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}

			[[nodiscard]] constexpr pointer operator->() const noexcept { return node; }
			[[nodiscard]] constexpr reference operator*() const noexcept { return *node; }

			[[nodiscard]] constexpr bool operator==(const list_iterator &) const noexcept = default;

			pointer node = nullptr;
		};

		typedef list_iterator iterator;
		typedef list_iterator const_iterator;
		typedef typename list_iterator::value_type value_type;
		typedef typename list_iterator::pointer pointer;
		typedef typename list_iterator::const_pointer const_pointer;
		typedef typename list_iterator::reference reference;
		typedef typename list_iterator::const_reference const_reference;
		typedef typename list_iterator::difference_type difference_type;
		typedef typename list_iterator::size_type size_type;

		[[nodiscard]] constexpr list_iterator begin() const noexcept { return {front}; }
		[[nodiscard]] constexpr list_iterator cbegin() const noexcept { return begin(); }
		[[nodiscard]] constexpr list_iterator end() const noexcept { return {}; }
		[[nodiscard]] constexpr list_iterator cend() const noexcept { return end(); }

		constexpr void insert(T &node) noexcept
		{
			node.next = front;
			front = &node;
		}

		const T *front = nullptr;
	};
	/* Custom view, as CLang has issues with `std::ranges::subrange` at this time. */
	template<typename Iter>
	class type_data_view
	{
	public:
		typedef typename Iter::value_type value_type;
		typedef typename Iter::pointer pointer;
		typedef typename Iter::const_pointer const_pointer;
		typedef typename Iter::reference reference;
		typedef typename Iter::const_reference const_reference;
		typedef typename Iter::difference_type difference_type;
		typedef typename Iter::size_type size_type;

		typedef Iter iterator;
		typedef Iter const_iterator;

	public:
		constexpr type_data_view() noexcept = default;
		constexpr type_data_view(iterator first, iterator last) noexcept : m_first(first), m_last(last) {}

		[[nodiscard]] constexpr iterator begin() const noexcept { return m_first; }
		[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
		[[nodiscard]] constexpr iterator end() const noexcept { return m_last; }
		[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

		// clang-format off
			[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
			[[nodiscard]] constexpr reference back() const noexcept requires std::bidirectional_iterator<Iter>
			{
				return *std::prev(end());
			}
			[[nodiscard]] constexpr reference at(size_type i) const noexcept requires std::random_access_iterator<Iter>
			{
				return begin()[static_cast<difference_type>(i)];
			}
			[[nodiscard]] constexpr reference operator[](size_type i) const noexcept requires std::random_access_iterator<Iter>
			{
				return at(i);
			}
		// clang-format on

		[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return static_cast<size_type>(std::distance(begin(), end()));
		}

		[[nodiscard]] constexpr bool operator==(const type_data_view &other) const noexcept
		{
			return std::equal(m_first, m_last, other.m_first, other.m_last);
		}

	private:
		Iter m_first = {};
		Iter m_last = {};
	};

	struct type_parent : type_data_node<type_parent>
	{
		any_ref (*cast)(any_ref);
		type_handle type;
	};
	struct type_conv : type_data_node<type_parent>
	{
		any (*convert)(any_ref);
		type_handle type;
	};

	struct type_attr : type_data_node<type_attr>
	{
		~type_attr()
		{
			if (destroy != nullptr) [[likely]]
				destroy(this);
		}

		any (*get)(const type_attr *) = nullptr;
		void (*destroy)(type_attr *) = nullptr;
		type_handle type;
	};
	struct type_enum : type_data_node<type_enum>
	{
		~type_enum()
		{
			if (destroy != nullptr) [[likely]]
				destroy(this);
		}

		any (*get)(const type_enum *) = nullptr;
		void (*destroy)(type_enum *) = nullptr;
		std::string_view name;
	};

	struct arg_type_data
	{
		type_handle type; /* Actual type of the argument. */
		bool is_const;	  /* Is the argument type const-qualified. */
	};
	using type_func_args = std::span<arg_type_data>;

	struct type_ctor : type_data_node<type_ctor>
	{
		~type_ctor()
		{
			if (destroy != nullptr) [[likely]]
				destroy(this);
		}

		any (*invoke)(const type_ctor *, std::span<any>) = nullptr;
		void (*destroy)(type_ctor *) = nullptr;
		type_func_args args;
	};
	struct type_func : type_data_node<type_func>
	{
		~type_func()
		{
			if (destroy != nullptr) [[likely]]
				destroy(this);
		}

		any (*invoke)(const type_func *, any, std::span<any>) = nullptr;
		void (*destroy)(type_func *) = nullptr;
		std::string_view name;
		type_func_args args;
		type_handle ret;
	};
	struct type_prop : type_data_node<type_prop>
	{
		~type_prop()
		{
			if (destroy != nullptr) [[likely]]
				destroy(this);
		}

		void (*set)(const type_prop *, any, any) = nullptr;
		any (*get)(const type_prop *, any) = nullptr;
		void (*destroy)(type_prop *) = nullptr;
		std::string_view name;
	};

	template<typename>
	struct range_type_iterator;
	template<typename>
	struct table_type_iterator;

	template<typename T>
	struct type_iterator_ptr
	{
		type_iterator_ptr(const type_iterator_ptr &) = delete;
		type_iterator_ptr &operator=(const type_iterator_ptr &) = delete;

		constexpr type_iterator_ptr() noexcept = default;
		constexpr type_iterator_ptr(type_iterator_ptr &&other) noexcept { swap(other); }
		constexpr type_iterator_ptr &operator=(type_iterator_ptr &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~type_iterator_ptr() { delete m_ptr; }

		template<typename U>
		constexpr type_iterator_ptr(U *ptr) noexcept : m_ptr(static_cast<T *>(ptr))
		{
		}

		template<typename U>
		void reset(U *ptr)
		{
			delete m_ptr;
			m_ptr = static_cast<T *>(ptr);
		}

		[[nodiscard]] constexpr operator bool() const noexcept { return m_ptr != nullptr; }

		[[nodiscard]] constexpr T *get() const noexcept { return m_ptr; }
		[[nodiscard]] constexpr T *operator->() const noexcept { return get() }
		[[nodiscard]] constexpr T &operator*() const noexcept { return *get() }

		constexpr void swap(type_iterator_ptr &other) { std::swap(m_ptr, other.m_ptr); }
		friend constexpr void swap(type_iterator_ptr &a, type_iterator_ptr &b) { a.swap(b); }

	private:
		T *m_ptr = nullptr;
	};
	template<>
	struct SEK_API range_type_iterator<void>
	{
		typedef any value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr range_type_iterator() noexcept = default;
		virtual ~range_type_iterator();

		[[nodiscard]] virtual range_type_iterator<void> *make_copy() const = 0;

		[[nodiscard]] virtual bool is_bidirectional() const noexcept = 0;
		[[nodiscard]] virtual bool is_random_access() const noexcept = 0;

		virtual void inc() = 0;
		virtual void inc(difference_type) = 0;
		virtual void dec() = 0;
		virtual void dec(difference_type) = 0;

		[[nodiscard]] virtual difference_type operator-(const range_type_iterator &) const = 0;

		[[nodiscard]] virtual any value() const = 0;

		[[nodiscard]] virtual bool operator==(const range_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator<(const range_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator<=(const range_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator>(const range_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator>=(const range_type_iterator &) const noexcept = 0;
	};
	template<>
	struct SEK_API table_type_iterator<void>
	{
		typedef any value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr table_type_iterator() noexcept = default;
		virtual ~table_type_iterator();

		[[nodiscard]] virtual table_type_iterator<void> *make_copy() const = 0;

		[[nodiscard]] virtual bool is_bidirectional() const noexcept = 0;
		[[nodiscard]] virtual bool is_random_access() const noexcept = 0;

		virtual void inc() = 0;
		virtual void inc(difference_type) = 0;
		virtual void dec() = 0;
		virtual void dec(difference_type) = 0;

		[[nodiscard]] virtual difference_type operator-(const table_type_iterator &) const = 0;

		[[nodiscard]] virtual any value() const = 0;

		[[nodiscard]] virtual bool operator==(const table_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator<(const table_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator<=(const table_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator>(const table_type_iterator &) const noexcept = 0;
		[[nodiscard]] virtual bool operator>=(const table_type_iterator &) const noexcept = 0;

		[[nodiscard]] virtual any key() const = 0;
		[[nodiscard]] virtual any mapped() const = 0;
	};

	struct range_type_data
	{
		template<typename T>
		constexpr static range_type_data make_instance() noexcept;
		template<typename T>
		static const range_type_data instance;

		type_handle value_type;

		bool (*empty)(const void *) = nullptr;
		std::size_t (*size)(const void *) = nullptr;

		type_iterator_ptr<range_type_iterator<void>> (*begin)(any_ref &) = nullptr;
		type_iterator_ptr<range_type_iterator<void>> (*cbegin)(const any_ref &) = nullptr;
		type_iterator_ptr<range_type_iterator<void>> (*end)(any_ref &) = nullptr;
		type_iterator_ptr<range_type_iterator<void>> (*cend)(const any_ref &) = nullptr;

		type_iterator_ptr<range_type_iterator<void>> (*rbegin)(any_ref &) = nullptr;
		type_iterator_ptr<range_type_iterator<void>> (*crbegin)(const any_ref &) = nullptr;
		type_iterator_ptr<range_type_iterator<void>> (*rend)(any_ref &) = nullptr;
		type_iterator_ptr<range_type_iterator<void>> (*crend)(const any_ref &) = nullptr;

		any (*front)(any_ref &) = nullptr;
		any (*cfront)(const any_ref &) = nullptr;
		any (*back)(any_ref &) = nullptr;
		any (*cback)(const any_ref &) = nullptr;
		any (*at)(any_ref &, std::size_t) = nullptr;
		any (*cat)(const any_ref &, std::size_t) = nullptr;
	};
	struct table_type_data
	{
		template<typename T>
		constexpr static table_type_data make_instance() noexcept;
		template<typename T>
		static const table_type_data instance;

		type_handle value_type;
		type_handle key_type;
		type_handle mapped_type;

		bool (*empty)(const void *) = nullptr;
		std::size_t (*size)(const void *) = nullptr;
		bool (*contains)(const void *, const any &) = nullptr;

		type_iterator_ptr<table_type_iterator<void>> (*find)(any_ref &, const any &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*cfind)(const any_ref &, const any &) = nullptr;

		type_iterator_ptr<table_type_iterator<void>> (*begin)(any_ref &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*cbegin)(const any_ref &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*end)(any_ref &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*cend)(const any_ref &) = nullptr;

		type_iterator_ptr<table_type_iterator<void>> (*rbegin)(any_ref &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*crbegin)(const any_ref &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*rend)(any_ref &) = nullptr;
		type_iterator_ptr<table_type_iterator<void>> (*crend)(const any_ref &) = nullptr;

		any (*at)(any_ref &, const any &) = nullptr;
		any (*cat)(const any_ref &, const any &) = nullptr;
	};
	struct tuple_type_data
	{
		template<typename T>
		struct getter_t
		{
			constexpr static auto size = std::tuple_size_v<T>;

			constexpr getter_t() noexcept { init(); }

			[[nodiscard]] any operator()(T &t, std::size_t i) const;

			template<std::size_t I = 0>
			constexpr void init() noexcept;

			any (*table[size])(T &);
		};

		template<typename T>
		constexpr static tuple_type_data make_instance() noexcept;
		template<typename T>
		constexpr static auto make_type_array() noexcept;
		template<typename T>
		static const tuple_type_data instance;

		std::span<type_handle> types;
		any (*get)(any_ref &, std::size_t) = nullptr;
		any (*cget)(const any_ref &, std::size_t) = nullptr;
	};
	struct string_type_data
	{
		template<typename T>
		constexpr static string_type_data make_instance() noexcept;
		template<typename T>
		static const string_type_data instance;

		type_handle char_type;
		type_handle traits_type;

		bool (*empty)(const void *) = nullptr;
		std::size_t (*size)(const void *) = nullptr;
		void *(*data)(any_ref &) = nullptr;
		const void *(*cdata)(const any_ref &) = nullptr;
	};

	struct any_funcs_t
	{
		template<typename T>
		constexpr static any_funcs_t make_instance() noexcept;

		void (*destroy)(any &) = nullptr;
		void (*construct)(const any &, any &) = nullptr;
		void (*assign)(const any &, any &) = nullptr;

		bool (*cmp_eq)(const void *, const void *) = nullptr;
		bool (*cmp_lt)(const void *, const void *) = nullptr;
		bool (*cmp_le)(const void *, const void *) = nullptr;
		bool (*cmp_gt)(const void *, const void *) = nullptr;
		bool (*cmp_ge)(const void *, const void *) = nullptr;
	};

	struct type_data
	{
		template<typename T>
		[[nodiscard]] constexpr static type_data make_instance() noexcept;
		template<typename T>
		static type_data *instance() noexcept;

		std::string_view name;
		bool is_void = false;
		bool is_empty = false;
		bool is_nullptr = false;

		type_handle enum_type;					  /* Underlying type of enum. */
		const type_conv *signed_conv = nullptr;	  /* Conversion to `std::intmax_t`. */
		const type_conv *unsigned_conv = nullptr; /* Conversion to `std::uintmax_t`. */
		const type_conv *floating_conv = nullptr; /* Conversion to `long double`. */

		any_funcs_t any_funcs = {};
		const range_type_data *range_data = nullptr;
		const table_type_data *table_data = nullptr;
		const tuple_type_data *tuple_data = nullptr;
		const string_type_data *string_data = nullptr;

		type_data_list<type_attr> attributes = {};
		type_data_list<type_enum> enumerations = {};

		type_data_list<type_parent> parents = {};
		type_data_list<type_conv> conversions = {};

		const type_ctor *default_ctor = nullptr;
		type_data_list<type_ctor> constructors = {};
		type_data_list<type_func> functions = {};
		type_data_list<type_prop> properties = {};
	};

	template<typename T>
	constexpr type_handle::type_handle(type_selector_t<T>) noexcept : get(type_data::instance<T>)
	{
	}
}	 // namespace sek::detail