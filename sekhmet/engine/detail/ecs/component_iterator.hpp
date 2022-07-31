/*
 * Created by switchblade on 30/07/22
 */

#pragma once

#include <tuple>

#include "sekhmet/detail/meta_util.hpp"

#include "fwd.hpp"

namespace sek::engine
{
	template<typename, typename...>
	class basic_component_iterator;

	namespace detail
	{
		template<typename... Cs>
		class group_pointer
		{
			template<typename, typename...>
			friend class sek::engine::basic_component_iterator;

			template<typename C, typename U>
			using valid_const = std::disjunction<std::is_const<C>, std::negation<std::is_const<U>>>;

			using ref_t = std::tuple<const entity_t &, Cs &...>;
			using ptr_t = std::tuple<const entity_t *, Cs *...>;

			template<std::size_t... Is, typename... Us>
			constexpr static ptr_t make_ptr(std::index_sequence<Is...>, const group_pointer<Us...> &other) noexcept
			{
				return ref_t{other.template get<Is>()...};
			}
			template<typename... Us>
			constexpr static ptr_t make_ptr(const group_pointer<Us...> &other) noexcept
			{
				return make_ptr(std::make_index_sequence<sizeof...(Us) + 1>{}, other);
			}

			template<typename... Args>
			constexpr explicit group_pointer(Args &&...args) noexcept : m_ptr(std::forward<Args>(args)...)
			{
			}

		public:
			constexpr group_pointer() noexcept = default;
			constexpr group_pointer(const group_pointer &) noexcept = default;
			constexpr group_pointer &operator=(const group_pointer &) noexcept = default;
			constexpr group_pointer(group_pointer &&) noexcept = default;
			constexpr group_pointer &operator=(group_pointer &&) noexcept = default;

			template<typename... Us, typename = std::enable_if_t<std::conjunction_v<valid_const<Cs, Us>...>>>
			constexpr group_pointer(const group_pointer<Us...> &other) noexcept : m_ptr(make_ptr(other))
			{
			}

			/** Returns pointer to the associated element. */
			template<typename T>
			[[nodiscard]] constexpr auto *get() noexcept
			{
				using std::get;
				return get<T>(m_ptr);
			}
			/** @copydoc get */
			template<std::size_t I>
			[[nodiscard]] constexpr auto *get() noexcept
			{
				using std::get;
				return get<I>(m_ptr);
			}
			/** @copydoc get */
			template<typename T>
			[[nodiscard]] constexpr const auto *get() const noexcept
			{
				using std::get;
				return get<T>(m_ptr);
			}
			/** @copydoc get */
			template<std::size_t I>
			[[nodiscard]] constexpr const auto *get() const noexcept
			{
				using std::get;
				return get<I>(m_ptr);
			}

			/** Returns a tuple of references to the associated entity and components. */
			[[nodiscard]] constexpr ref_t operator*() const noexcept { return m_ref; }
			/** Returns pointer to a tuple of references to the associated entity and components. */
			[[nodiscard]] constexpr const ref_t *get() const noexcept { return &m_ref; }
			/** @copydoc get */
			[[nodiscard]] constexpr const ref_t *operator->() const noexcept { return get(); }

			[[nodiscard]] constexpr auto operator<=>(const group_pointer &other) const noexcept
			{
				return m_ref.m_ptr <=> other.m_ref.m_ptr;
			}
			[[nodiscard]] constexpr bool operator==(const group_pointer &other) const noexcept
			{
				return m_ref.m_ptr == other.m_ref.m_ptr;
			}

		private:
			union
			{
				/* Union of references and pointers, used to handle `nullptr` conditions. */
				ptr_t m_ptr;
				ref_t m_ref;
			};
		};
	}	 // namespace detail

	/** @brief Iterator mixin used to iterate over a group of component pools.
	 * @tparam Base Type used as the mixin base for the component iterator.
	 * @tparam Cs Types of iterated components. */
	template<typename Base, typename... Cs>
	class basic_component_iterator : Base
	{
		template<typename, typename, typename>
		friend class component_view;
		template<typename...>
		friend class basic_component_collection;

		template<typename C, typename U>
		using valid_const = std::disjunction<std::is_const<C>, std::negation<std::is_const<U>>>;
		template<typename T>
		using set_t = transfer_cv_t<T, component_set<std::remove_cv_t<T>>>;
		using ptr_t = std::tuple<set_t<Cs> *...>;

		template<std::size_t... Is, typename... Us>
		constexpr static ptr_t make_ptr(std::index_sequence<Is...>, const basic_component_iterator<Base, Us...> &other) noexcept
		{
			using std::get;
			return ptr_t{get<Is>(other.m_ptr)...};
		}
		template<typename... Us>
		constexpr static ptr_t make_ptr(const basic_component_iterator<Base, Us...> &other) noexcept
		{
			return make_ref(std::make_index_sequence<sizeof...(Us) + 1>{}, other);
		}

		constexpr basic_component_iterator(Base base, set_t<Cs> *...ptr) noexcept : Base(base), m_ptr(ptr...) {}
		constexpr basic_component_iterator(Base base, const basic_component_iterator &other) noexcept
			: Base(base), m_ptr(other.m_ptr)
		{
		}

	public:
		typedef std::tuple<entity_t, Cs...> value_type;
		/** @brief Tuple-like fancy pointer type used to reference an entity and a set of components. */
		typedef detail::group_pointer<Cs...> pointer;
		/** @brief Tuple of references to the entity it's components. */
		typedef std::tuple<const entity_t &, Cs &...> reference;

		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::bidirectional_iterator_tag iterator_category;

	public:
		constexpr basic_component_iterator() noexcept = default;

		template<typename... Us, typename = std::enable_if_t<std::conjunction_v<valid_const<Cs, Us>...>>>
		constexpr basic_component_iterator(const basic_component_iterator<Base, Us...> &other) noexcept
			: Base(other), m_ptr(make_ptr(other))
		{
		}

		constexpr basic_component_iterator operator++(int) noexcept
		{
			auto temp = *this;
			++(*this);
			return temp;
		}
		constexpr basic_component_iterator &operator++() noexcept
		{
			Base::operator+=(1);
			return *this;
		}
		constexpr basic_component_iterator &operator+=(difference_type n) noexcept
		{
			Base::operator+=(n);
			return *this;
		}
		constexpr basic_component_iterator operator--(int) noexcept
		{
			auto temp = *this;
			--(*this);
			return temp;
		}
		constexpr basic_component_iterator &operator--() noexcept
		{
			Base::operator-=(1);
			return *this;
		}
		constexpr basic_component_iterator &operator-=(difference_type n) noexcept
		{
			Base::operator-=(n);
			return *this;
		}

		[[nodiscard]] constexpr basic_component_iterator operator+(difference_type n) const noexcept
		{
			return basic_component_iterator{Base::operator+(n), *this};
		}
		[[nodiscard]] constexpr basic_component_iterator operator-(difference_type n) const noexcept
		{
			return basic_component_iterator{Base::operator-(n), *this};
		}
		[[nodiscard]] constexpr difference_type operator-(const basic_component_iterator &other) const noexcept
		{
			return Base::operator-(other);
		}

		/** Returns tuple-like fancy pointer to the associated entity and components. */
		[[nodiscard]] constexpr pointer get() const noexcept { return get(std::make_index_sequence<sizeof...(Cs)>{}); }
		/** @copydoc get */
		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		/** Returns tuple-like structure containing references to the associated entity and components.
		 * @note If the iterator allows optional components, may result in `nullptr` dereference. */
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
		/** Equivalent to `*(this->operator+(n))`.
		 * @note If the iterator allows optional components, may result in `nullptr` dereference. */
		[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return *operator+(n); }

		[[nodiscard]] constexpr auto operator<=>(const basic_component_iterator &other) const noexcept
		{
			return Base::operator<=>(other);
		}
		[[nodiscard]] constexpr bool operator==(const basic_component_iterator &other) const noexcept
		{
			return Base::operator==(other);
		}

		constexpr void swap(basic_component_iterator &other) noexcept
		{
			Base::swap(other);
			std::swap(m_ptr, other.m_ptr);
		}
		friend constexpr void swap(basic_component_iterator &a, basic_component_iterator &b) noexcept { a.swap(b); }

	private:
		template<std::size_t... Is>
		[[nodiscard]] constexpr pointer get(std::index_sequence<Is...>) const noexcept
		{
			using std::get;
			return pointer{Base::get(), Base::get(get<Is>(m_ptr))...};
		}

		ptr_t m_ptr;
	};

	template<typename T, typename B, typename... Cs>
	[[nodiscard]] constexpr auto *get(sek::engine::detail::group_pointer<B, Cs...> &p) noexcept
	{
		return p.template get<T>();
	}
	template<typename T, typename B, typename... Cs>
	[[nodiscard]] constexpr auto *get(const sek::engine::detail::group_pointer<B, Cs...> &p) noexcept
	{
		return p.template get<T>();
	}
	template<std::size_t I, typename B, typename... Cs>
	[[nodiscard]] constexpr auto *get(sek::engine::detail::group_pointer<B, Cs...> &p) noexcept
	{
		return p.template get<I>();
	}
	template<std::size_t I, typename B, typename... Cs>
	[[nodiscard]] constexpr auto *get(const sek::engine::detail::group_pointer<B, Cs...> &p) noexcept
	{
		return p.template get<I>();
	}
}	 // namespace sek::engine

template<typename B, typename... Cs>
struct std::tuple_size<sek::engine::detail::group_pointer<B, Cs...>> : std::integral_constant<std::size_t, sizeof...(Cs) + 1>
{
};