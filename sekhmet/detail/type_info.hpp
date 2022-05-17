//
// Created by switchblade on 2022-01-23.
//

#pragma once

#include "define.h"
#include "meta_containers.hpp"
#include "static_string.hpp"

namespace sek
{
	class type_info;

	namespace detail
	{
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name(basic_static_string<char, N> result) noexcept
		{
			if constexpr (I == Last)
			{
				result[J] = '\0';
				return result;
			}
			else
			{
				result[J] = static_cast<typename decltype(result)::value_type>(Src[I]);
				return format_type_name<Src, J + 1, I + 1, Last>(result);
			}
		}
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name() noexcept
		{
			return format_type_name<Src, J, I, Last, N>({});
		}
		template<basic_static_string Name>
		consteval auto generate_type_name_impl() noexcept
		{
#if defined(__clang__) || defined(__GNUC__)
			constexpr auto offset_start = Name.find_first('=') + 2;
			constexpr auto offset_end = Name.find_last(']');
			constexpr auto trimmed_length = offset_end - offset_start + 1;
#elif defined(_MSC_VER)
			constexpr auto offset_start = Name.find_first('<') + 1;
			constexpr auto offset_end = Name.find_last('>');
			constexpr auto trimmed_length = offset_end - offset_start + 1;
#else
#error "Implement type name generation for this compiler"
#endif
			return format_type_name<Name, 0, offset_start, offset_end, trimmed_length>();
		}

		template<typename T>
		[[nodiscard]] constexpr std::basic_string_view<char> generate_type_name() noexcept
		{
			constexpr auto &value = auto_constant<generate_type_name_impl<SEK_PRETTY_FUNC>()>::value;
			return std::basic_string_view<char>{value.begin(), value.end()};
		}
	}	 // namespace detail

	/** Returns name of the specified type.
	 * @warning Consistency of generated type names across different compilers is not guaranteed.
	 * To generate consistent type names, overload this function for the desired type. */
	template<typename T>
	[[nodiscard]] constexpr std::string_view type_name() noexcept
	{
		return detail::generate_type_name<T>();
	}

	/** @brief Structure used to reference reflected information of a type. */
	class type_info
	{
		struct type_data;
		struct type_handle
		{
			constexpr type_handle() noexcept = default;

			template<typename T>
			constexpr explicit type_handle(type_selector_t<T>) noexcept : instance(type_data::instance<T>)
			{
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return instance == nullptr; }
			[[nodiscard]] constexpr type_data *operator->() const noexcept { return instance(); }
			[[nodiscard]] constexpr type_data &operator*() const noexcept { return *instance(); }

			[[nodiscard]] constexpr bool operator==(const type_handle &) const noexcept;

			type_data *(*instance)() noexcept = nullptr;
		};
		struct type_data
		{
			template<typename>
			static type_data *instance() noexcept;

			struct metadata_node
			{
				const metadata_node *next = nullptr;
			};

			struct parent_node : metadata_node, type_handle
			{
				template<typename T>
				constexpr explicit parent_node(type_selector_t<T>) noexcept : type_handle(type_selector<T>)
				{
				}
			};
			struct attrib_node : metadata_node
			{
				constexpr attrib_node() noexcept = default;
				constexpr ~attrib_node()
				{
					if (destroy) destroy(this);
				}

				type_handle type;
				std::string_view key;
				void (*destroy)(attrib_node *) = nullptr;
				const void *data;
			};

			template<std::derived_from<metadata_node> T>
			struct metadata_list
			{
				struct iterator
				{
					typedef T value_type;
					typedef const T *pointer;
					typedef const T &reference;
					typedef std::ptrdiff_t difference_type;
					typedef std::size_t size_type;
					typedef std::forward_iterator_tag iterator_category;

					constexpr iterator() noexcept = default;
					constexpr explicit iterator(const T *node) noexcept : node(node) {}

					constexpr iterator &operator++() noexcept
					{
						node = static_cast<const T *>(node->next);
						return *this;
					}
					constexpr iterator operator++(int) noexcept
					{
						auto temp = *this;
						++(*this);
						return temp;
					}

					constexpr pointer operator->() const noexcept { return node; }
					constexpr reference operator*() const noexcept { return *node; }

					constexpr bool operator==(const iterator &) const noexcept = default;

					const T *node = nullptr;
				};

				constexpr void insert(T &node) noexcept
				{
					node.next = first;
					first = std::addressof(node);
				}

				constexpr iterator begin() const noexcept { return iterator{first}; }
				constexpr iterator end() const noexcept { return iterator{}; }

				const T *first = nullptr;
			};

			template<typename T>
			constexpr explicit type_data(type_selector_t<T>) noexcept
				: name(type_name<T>()), size(sizeof(T)), align(alignof(T))
			{
			}

			[[nodiscard]] constexpr const parent_node *get_parent(std::string_view n) const noexcept
			{
				for (auto &node : parent_list)
					if (auto *result = &node; node->name == n || (result = node->get_parent(n)) != nullptr)
						return result;
				return nullptr;
			}
			[[nodiscard]] constexpr const attrib_node *get_attribute(std::string_view key) const noexcept
			{
				for (auto &node : attrib_list)
					if (node.key == key) return &node;
				return nullptr;
			}

			const std::string_view name;
			const std::size_t size;
			const std::size_t align;

			metadata_list<parent_node> parent_list;
			metadata_list<attrib_node> attrib_list;
		};

		template<typename T>
		class type_factory
		{
			friend class type_info;

			constexpr explicit type_factory(type_data *data) noexcept : data(data) {}

		public:
			type_factory() = delete;

			constexpr type_factory(const type_factory &) noexcept = default;
			constexpr type_factory &operator=(const type_factory &) noexcept = default;
			constexpr type_factory(type_factory &&) noexcept = default;
			constexpr type_factory &operator=(type_factory &&) noexcept = default;

			// clang-format off
			/** Adds `U` to the list of type's parents. */
			template<typename U>
			type_factory parent() noexcept requires std::derived_from<T, U>
			{
				constinit static auto node = type_data::parent_node{type_selector<U>};
				if (!node.next) [[likely]]
					data->parent_list.insert(node);
				return *this;
			}
			// clang-format on

		private:
			type_data *data;
		};

		SEK_API static type_data *reflect_impl(type_handle);

	public:
		/** Returns type info of `T`. */
		template<typename T>
		constexpr static type_info get() noexcept
		{
			return type_info{type_selector<T>};
		}

		/** Reflects type `T` and returns type factory for it.
		 * A reflected type is accessible through the type database using it's name. */
		template<typename T>
		static type_factory<T> reflect()
		{
			return type_factory<T>{reflect_impl(type_handle{type_selector<T>})};
		}
		/** Returns type info of a reflected type.
		 * @return Type info of the reflected type. If such type was not reflected via `reflect`, returns an empty type info. */
		SEK_API static type_info get(std::string_view name) noexcept;
		/** Resets a reflected type.
		 * @note Once a type is reset, it is no longer accessible via the non-template overload of `get`. */
		SEK_API static void reset(std::string_view name) noexcept;
		/** @copydoc reset */
		template<typename T>
		static void reset() noexcept
		{
			reset(type_name<T>());
		}

	private:
		template<typename T>
		constexpr explicit type_info(type_selector_t<T>) noexcept : handle(type_selector<T>)
		{
		}
		constexpr explicit type_info(type_handle handle) noexcept : handle(handle) {}

	public:
		/** Returns name of the underlying type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return handle->name; }
		/** Returns size of the underlying type. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return handle->size; }
		/** Returns alignment of the underlying type. */
		[[nodiscard]] constexpr std::size_t align() const noexcept { return handle->align; }

		/** Checks if the underlying type inherits from the specified parent type. */
		[[nodiscard]] constexpr bool inherits(std::string_view name) const noexcept
		{
			return handle->get_parent(name) != nullptr;
		}
		/** Checks if `T` is a parent of the underlying type. */
		template<typename T>
		[[nodiscard]] constexpr bool inherits() const noexcept
		{
			return inherits(type_name<T>());
		}

		[[nodiscard]] constexpr bool operator==(const type_info &) const noexcept = default;

	private:
		type_handle handle;
	};

	template<typename T>
	type_info::type_data *type_info::type_data::instance() noexcept
	{
		constinit static auto value = type_data{type_selector<T>};
		return &value;
	}

	constexpr bool sek::type_info::type_handle::operator==(const type_handle &other) const noexcept
	{
		return instance()->name == other.instance()->name;
	}
}	 // namespace sek

/** Macro used to declare an instance of type info for type `T` as extern.
 *
 * @note Type must be exported via `SEK_EXPORT_TYPE`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE(my_type)
 * @endcode*/
#define SEK_EXTERN_TYPE(T)                                                                                             \
	extern template SEK_API_IMPORT sek::type_info::type_data *sek::type_info::type_data::instance<T>();

/** Macro used to export instance of type info for type `T`.
 *
 * @note Type must be declared as `extern` via `SEK_EXTERN_TYPE`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE(my_type)
 * @endcode */
#define SEK_EXPORT_TYPE(T) template SEK_API_EXPORT sek::type_info::type_data *sek::type_info::type_data::instance<T>();
