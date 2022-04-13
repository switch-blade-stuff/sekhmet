//
// Created by switch_blade on 2022-04-12.
//

#pragma once

#include "meta_containers.hpp"
#include "meta_util.hpp"
#include "type_id.hpp"

namespace sek::detail
{
	struct type_data
	{
		template<typename T>
		struct node_t
		{
			constexpr explicit node_t(T &&data) noexcept : data(std::forward<T>(data)) {}

			const node_t *next = nullptr;
			T data;
		};

		struct handle_t
		{
			typedef const type_data *(*getter_func)() noexcept;

			constexpr handle_t() noexcept = default;
			constexpr explicit handle_t(getter_func getter) noexcept : getter(getter) {}

			[[nodiscard]] constexpr bool empty() const noexcept { return getter == nullptr; }

			[[nodiscard]] const type_data *get() const noexcept { return getter(); }
			[[nodiscard]] const type_data *operator->() const noexcept { return get(); }
			[[nodiscard]] const type_data &operator*() const noexcept { return *get(); }

			getter_func getter;
		};

		template<typename T>
		constexpr static handle_t make_handle() noexcept;

		struct attribute_t
		{
			template<typename T>
			constexpr explicit attribute_t(const T *data) noexcept
				: type(make_handle<T>()), data(static_cast<const void *>(data))
			{
			}

			handle_t type;
			const void *data;
		};

		template<auto V>
		constexpr static attribute_t make_attribute() noexcept
		{
			return attribute_t{&auto_constant<V>::value};
		}

		template<typename T>
		constexpr explicit type_data(type_selector_t<T>) noexcept : tid(type_id::identify<T>())
		{
		}

		[[nodiscard]] constexpr bool inherits(type_id id) const noexcept
		{
			for (auto parent = parent_list; parent != nullptr; parent = parent->next)
				if (parent->data->tid == id || parent->data->inherits(id)) [[unlikely]]
					return true;
			return false;
		}
		[[nodiscard]] constexpr std::size_t has_attribute(type_id id) const noexcept
		{
			std::size_t result = 0;
			for (auto attrib = attribute_list; attrib != nullptr; attrib = attrib->next)
				if (attrib->data.type->tid == id) [[unlikely]]
					++result;
			return result;
		}

		type_id tid;

		const node_t<handle_t> *parent_list = nullptr;
		const node_t<attribute_t> *attribute_list = nullptr;
	};

	template<typename F, typename T>
	struct type_factory_base
	{
		friend struct type_data;

		template<typename U, typename... Us>
		void parents() noexcept requires(std::is_base_of_v<U, T> && !std::is_same_v<U, T>)
		{
			constinit static type_data::node_t<type_data::handle_t> parent{type_data::make_handle<U>()};
			if (!parent.next) [[likely]] /* If next is not null, this parent is already added. */
				parent.next = std::exchange(result.parent_list, &parent);

			if constexpr (sizeof...(Us) != 0) parents<Us...>();
		}

		template<auto V, auto... Vs>
		void attributes() noexcept
		{
			constinit static type_data::node_t<type_data::attribute_t> attribute{type_data::make_attribute<V>()};
			if (!attribute.next) [[likely]] /* If next is not null, this attribute is already added. */
				attribute.next = std::exchange(result.attribute_list, &attribute);

			if constexpr (sizeof...(Vs) != 0) attributes<Vs...>();
		}

	private:
		static const type_data *get_instance() noexcept
		{
			static type_data instance = F{}.result;
			return &instance;
		}

		type_data result = type_data{type_selector<T>};
	};
}	 // namespace sek::detail

namespace instantiation
{
	template<typename T>
	struct type_factory : sek::detail::type_factory_base<type_factory<T>, T>
	{
		type_factory() noexcept : sek::detail::type_factory_base<type_factory<T>, T>() { invoke(); }

		void invoke() noexcept;
	};
	template<typename T>
	void type_factory<T>::invoke() noexcept
	{
		/* Default factory does nothing. */
	}
}	 // namespace instantiation

namespace sek
{
	template<typename T>
	constexpr detail::type_data::handle_t detail::type_data::make_handle() noexcept
	{
		return handle_t{instantiation::type_factory<T>::get_instance};
	}

	/** @brief Structure used to reference reflected information about a type. */
	class type_info
	{
	public:
		/** Returns type info of `std::decay_t<T>`. */
		template<typename T>
		[[nodiscard]] constexpr static type_info get() noexcept
		{
			return type_info{detail::type_data::make_handle<std::decay_t<T>>()};
		}

	private:
		template<typename Value, typename Data>
		class info_iterator
		{
			friend class type_info;

		public:
			typedef Value value_type;
			typedef Value reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			using node_t = const detail::type_data::node_t<Data> *;

			constexpr explicit info_iterator(node_t node) noexcept : node(node) {}

		public:
			constexpr info_iterator() noexcept = default;

			constexpr info_iterator &operator++() noexcept
			{
				node = node->next;
				return *this;
			}
			constexpr info_iterator operator++(int) noexcept
			{
				auto temp = *this;
				operator++();
				return temp;
			}

			[[nodiscard]] constexpr value_type operator*() const noexcept { return value_type{node->data}; }

			[[nodiscard]] constexpr bool operator==(const info_iterator &) const noexcept = default;

		private:
			node_t node = nullptr;
		};

	public:
		class attribute
		{
			template<typename, typename>
			friend class info_iterator;

			constexpr explicit attribute(const detail::type_data::attribute_t &ptr) noexcept : ptr(&ptr) {}

		public:
			/** Initializes an empty attribute. */
			constexpr attribute() noexcept = default;

			/** Checks if the attribute is empty. */
			[[nodiscard]] constexpr bool empty() const noexcept { return ptr == nullptr; }

			/** Returns attribute's type. */
			[[nodiscard]] constexpr type_info type() const noexcept;

			/** Returns void pointer to attribute's data. */
			[[nodiscard]] constexpr const void *data() const noexcept { return ptr->data; }
			/** Returns pointer to attribute as const pointer to `T`.
			 * @note If the attribute is not of type `T`, returns nullptr. */
			template<typename T>
			[[nodiscard]] constexpr const T *data() const noexcept
			{
				constexpr auto id = type_id::identify<T>();
				if (ptr->type->tid != id) [[unlikely]]
					return nullptr;
				else
					return static_cast<const T *>(ptr->data);
			}

		private:
			const detail::type_data::attribute_t *ptr = nullptr;
		};

	private:
		using parent_iterator = info_iterator<type_info, detail::type_data::handle_t>;
		using attribute_iterator = info_iterator<attribute, detail::type_data::attribute_t>;

		constexpr explicit type_info(detail::type_data::handle_t handle) noexcept : handle(handle) {}

	public:
		/** Initializes an empty type info. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info is empty (does not reference any type). */
		[[nodiscard]] bool empty() const noexcept { return handle.empty(); }
		/** Returns type ID of the type. */
		[[nodiscard]] type_id tid() const noexcept { return handle->tid; }

		/** Returns a range containing parents of this type. */
		[[nodiscard]] auto parents() const noexcept;
		/** Checks if the type inherits from `id` type. */
		[[nodiscard]] bool inherits(type_id id) const noexcept { return handle->inherits(id); }
		/** Checks if the type inherits from `T`. */
		template<typename T>
		[[nodiscard]] bool inherits() const noexcept
		{
			constexpr auto id = type_id::identify<std::decay_t<T>>();
			return inherits(id);
		}

		/** Returns a range containing attributes of this type. */
		[[nodiscard]] auto attributes() const noexcept;
		/** Checks if the type has an attribute of type `id`.
		 * @return Amount of attributes who's type is `id`. */
		[[nodiscard]] std::size_t has_attribute(type_id id) const noexcept { return handle->has_attribute(id); }
		/** Checks if the type has an attribute of `T`.
		 * @return Amount of attributes who's type is `T`. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<std::decay_t<T>>();
			return has_attribute(id);
		}

	private:
		detail::type_data::handle_t handle;
	};

	constexpr type_info type_info::attribute::type() const noexcept { return type_info{ptr->type}; }

	auto type_info::parents() const noexcept
	{
		using range_t = std::ranges::subrange<parent_iterator, parent_iterator>;
		return range_t{parent_iterator{handle->parent_list}, parent_iterator{}};
	}
	auto type_info::attributes() const noexcept
	{
		using range_t = std::ranges::subrange<attribute_iterator, attribute_iterator>;
		return range_t{attribute_iterator{handle->attribute_list}, attribute_iterator{}};
	}
}	 // namespace sek

#define SEK_DETAIL_REFLECT_TYPE_2(T, name)                                                                             \
	namespace sek::detail                                                                                              \
	{                                                                                                                  \
		template<>                                                                                                     \
		constexpr std::string_view generate_type_name<T>() noexcept                                                    \
		{                                                                                                              \
			constexpr auto &value = auto_constant<basic_static_string{(name)}>::value;                                 \
			return std::string_view{value.begin(), value.end()};                                                       \
		}                                                                                                              \
	}                                                                                                                  \
	template<>                                                                                                         \
	void ::instantiation::type_factory<T>::invoke() noexcept

#define SEK_DETAIL_REFLECT_TYPE_1(T) SEK_DETAIL_REFLECT_TYPE_2(T, #T)

/** Macro used to specialize a type info factory. Must be placed in the same header as the reflected type.
 *
 * Accepts an optional name parameter used to set a custom type ID for the declared type.
 * If a custom ID is not provided, uses the first parameter as the ID.
 *
 * @note Type factory is invoked whenever an instance of `type_data` is instantiated for the type.
 *
 * @example
 * ```cpp
 * // my_type.hpp
 *
 * struct my_type : my_base_type
 * {
 * };
 *
 * SEK_REFLECT_TYPE(my_type)
 * {
 * 	parents<my_base_type>();
 * }
 * ```*/
#define SEK_REFLECT_TYPE(...)                                                                                          \
	SEK_GET_MACRO_2(__VA_ARGS__, SEK_DETAIL_REFLECT_TYPE_2, SEK_DETAIL_REFLECT_TYPE_1)(__VA_ARGS__)
