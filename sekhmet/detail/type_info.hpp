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
		struct type_db; /* Implemented in `type_info.cpp`. */

		struct type_data;
		struct type_handle
		{
			constexpr type_handle() noexcept = default;

			template<typename T>
			constexpr explicit type_handle(type_selector_t<T>) noexcept : instance(type_data::instance<T>)
			{
			}

			[[nodiscard]] constexpr bool valid() const noexcept { return instance == nullptr; }
			[[nodiscard]] constexpr type_data *operator->() const noexcept { return instance(); }
			[[nodiscard]] constexpr type_data &operator*() const noexcept { return *instance(); }

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
			struct attrib_node_base : metadata_node
			{
				constexpr attrib_node_base(type_handle type, const void *data) noexcept : type(type), data(data) {}
				constexpr ~attrib_node_base()
				{
					if (destroy) destroy(this);
				}

				/* Since attribute is type-agnostic, proxy destructor is needed. */
				void (*destroy)(attrib_node_base *) = nullptr;
				type_handle type;
				const void *data;
			};

			template<typename T>
			struct attrib_node : attrib_node_base
			{
				constexpr attrib_node() noexcept : attrib_node_base(type_handle{type_selector<T>}, &value), bytes{} {}

				template<typename... Args>
				constexpr void init(Args &&...args)
				{
					std::construct_at(&value, std::forward<Args>(args)...);
					if constexpr (!std::is_trivially_destructible_v<T>)
						destroy = +[](attrib_node_base *n) { std::destroy_at(&static_cast<attrib_node *>(n)->value); };
				}

				union
				{
					std::byte bytes[sizeof(T)];
					T value;
				};
			};
			// clang-format off
			template<typename T> requires std::is_empty_v<T> && std::is_trivially_constructible_v<T>
			struct attrib_node<T> : attrib_node_base
			{
				constexpr attrib_node() noexcept : attrib_node_base(type_handle{type_selector<T>}, nullptr) {}
				template<typename... Args>
				constexpr void init(Args &&...) {}
			};
			// clang-format on

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
			constexpr static bool is_qualified = !std::is_same_v<std::remove_cv_t<T>, T>;

			enum flags_t : std::size_t
			{
				NO_FLAGS = 0,
				EMPTY_TYPE = 1,
				ARRAY_TYPE = 2,
				POINTER_TYPE = 8,

				QUALIFIED_TYPE = 16,
				CONST_TYPE = QUALIFIED_TYPE | 32,
				VOLATILE_TYPE = QUALIFIED_TYPE | 64,

				SIGNED_TYPE = 128,
				FUNDAMENTAL_TYPE = 256,
				INTEGRAL_TYPE = FUNDAMENTAL_TYPE | 512,
				FLOATING_TYPE = FUNDAMENTAL_TYPE | SIGNED_TYPE | 1024,
			};

			template<typename T>
			constexpr static flags_t make_flags() noexcept
			{
				flags_t result = NO_FLAGS;

				if constexpr (std::is_empty_v<T>) result = static_cast<flags_t>(result | EMPTY_TYPE);
				if constexpr (std::is_array_v<T>) result = static_cast<flags_t>(result | ARRAY_TYPE);
				if constexpr (std::is_pointer_v<T>) result = static_cast<flags_t>(result | POINTER_TYPE);
				if constexpr (std::is_const_v<T>) result = static_cast<flags_t>(result | CONST_TYPE);
				if constexpr (std::is_volatile_v<T>) result = static_cast<flags_t>(result | VOLATILE_TYPE);

				if constexpr (std::is_signed_v<T>) result = static_cast<flags_t>(result | SIGNED_TYPE);
				if constexpr (std::is_floating_point_v<T>)
					result = static_cast<flags_t>(result | FLOATING_TYPE);
				else if constexpr (std::is_integral_v<T>)
					result = static_cast<flags_t>(result | INTEGRAL_TYPE);
				else if constexpr (std::is_fundamental_v<T>)
					result = static_cast<flags_t>(result | FUNDAMENTAL_TYPE);

				return result;
			}

			template<typename T>
			constexpr explicit type_data(type_selector_t<T>) noexcept
				: name(type_name<T>()),
				  size(sizeof(T)),
				  align(alignof(T)),
				  extent(std::extent_v<T>),
				  flags(make_flags<T>()),
				  unqualified(type_selector<std::remove_cv_t<T>>)
			{
				if constexpr (std::is_array_v<T>)
					std::construct_at(&remove_extent, type_selector<std::remove_extent_t<T>>);
				else if constexpr (std::is_pointer_v<T>)
					std::construct_at(&remove_pointer, type_selector<std::remove_pointer_t<T>>);
			}

			[[nodiscard]] constexpr const parent_node *get_parent(std::string_view n) const noexcept
			{
				for (auto &node : parent_list)
					if (auto *result = &node; node->name == n || (result = node->get_parent(n)) != nullptr)
						return result;
				return nullptr;
			}

			const std::string_view name;
			const std::size_t size;
			const std::size_t align;
			const std::size_t extent;
			const flags_t flags;
			const type_handle unqualified;

			union
			{
				type_handle remove_extent = {};
				type_handle remove_pointer;
			};

			metadata_list<parent_node> parent_list;
			metadata_list<attrib_node_base> attrib_list;
		};

		template<typename, typename>
		class type_factory;

		template<typename T, typename... Attr>
		class type_factory<T, type_seq_t<Attr...>>
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

			/** Adds attribute of type `U` to the list of type's attributes.
			 * @param args Arguments used to construct the attribute. */
			template<typename U, typename... Args>
			type_factory<T, type_seq_t<Attr..., U>> attrib(Args &&...args) noexcept
			{
				constinit static type_data::attrib_node<U> node;
				if (!node.next) [[likely]]
				{
					node.init(std::forward<Args>(args)...);
					data->attrib_list.insert(node);
				}
				return type_factory<T, type_seq_t<Attr..., U>>{data};
			}

		private:
			type_data *data;
		};

		SEK_API static type_data *reflect(type_handle);

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
		static type_factory<T, type_seq_t<>> reflect()
		{
			return type_factory<T, type_seq_t<>>{reflect(type_handle{type_selector<T>})};
		}
		/** Returns type info of a reflected type.
		 * @return Type info of the reflected type. If such type was not reflected via `reflect`, returns an invalid type info. */
		SEK_API static type_info get(std::string_view name);
		/** Resets a reflected type.
		 * @note Once a type is reset, it is no longer accessible via the non-template overload of `get`. */
		SEK_API static void reset(std::string_view name);
		/** @copydoc reset */
		template<typename T>
		static void reset()
		{
			reset(type_name<T>());
		}

	private:
		template<typename T>
		constexpr explicit type_info(type_selector_t<T>) noexcept : type_info(type_handle{type_selector<T>})
		{
		}
		constexpr explicit type_info(type_handle handle) noexcept : data(handle.instance ? handle.instance() : nullptr)
		{
		}

	public:
		/** Constructs an invalid type info (type info not representing any type). */
		constexpr type_info() noexcept = default;

		/** Checks if the type info is valid. */
		[[nodiscard]] constexpr bool valid() const noexcept { return data != nullptr; }
		/** @copydoc valid */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns name of the underlying type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return data->name; }
		/** Returns size of the underlying type. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return data->size; }
		/** Returns alignment of the underlying type. */
		[[nodiscard]] constexpr std::size_t align() const noexcept { return data->align; }
		/** Returns extent of the underlying fixed-size array type. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept { return data->extent; }
		/** Returns value type of the underlying array type. */
		[[nodiscard]] constexpr type_info remove_extent() const noexcept { return type_info{data->remove_extent}; }
		/** Returns value type of the underlying pointer type. */
		[[nodiscard]] constexpr type_info remove_pointer() const noexcept { return type_info{data->remove_pointer}; }
		/** Returns unqualified version of the underlying type.
		 * @note If the type is not qualified, returns itself. */
		[[nodiscard]] constexpr type_info remove_cv() const noexcept { return type_info{data->unqualified}; }

		/** Checks if the underlying type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return data->flags & type_data::EMPTY_TYPE; }
		/** Checks if the underlying type is an array. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return data->flags & type_data::ARRAY_TYPE; }
		/** Checks if the underlying type is a pointer type. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return data->flags & type_data::POINTER_TYPE; }
		/** Checks if the underlying type is qualified with either const or volatile. */
		[[nodiscard]] constexpr bool is_qualified() const noexcept { return data->flags & type_data::QUALIFIED_TYPE; }
		/** Checks if the underlying type is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept
		{
			return (data->flags & type_data::CONST_TYPE) == type_data::CONST_TYPE;
		}
		/** Checks if the underlying type is volatile-qualified. */
		[[nodiscard]] constexpr bool is_volatile() const noexcept
		{
			return (data->flags & type_data::VOLATILE_TYPE) == type_data::VOLATILE_TYPE;
		}
		/** Checks if the underlying type is a signed type. */
		[[nodiscard]] constexpr bool is_signed() const noexcept { return data->flags & type_data::SIGNED_TYPE; }
		/** Checks if the underlying type is a fundamental type. */
		[[nodiscard]] constexpr bool is_fundamental() const noexcept
		{
			return data->flags & type_data::FUNDAMENTAL_TYPE;
		}
		/** Checks if the underlying type is an integral type. */
		[[nodiscard]] constexpr bool is_integral() const noexcept { return data->flags & type_data::INTEGRAL_TYPE; }
		/** Checks if the underlying type is a floating-point type. */
		[[nodiscard]] constexpr bool is_floating_point() const noexcept
		{
			return data->flags & type_data::FLOATING_TYPE;
		}

		/** Checks if the underlying type inherits from the specified parent type. */
		[[nodiscard]] constexpr bool inherits(std::string_view name) const noexcept
		{
			return data->get_parent(name) != nullptr;
		}
		/** Checks if `T` is a parent of the underlying type. */
		template<typename T>
		[[nodiscard]] constexpr bool inherits() const noexcept
		{
			return inherits(type_name<T>());
		}

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			return data == other.data || name() == other.name();
		}

	private:
		const type_data *data = nullptr;
	};

	template<typename T>
	type_info::type_data *type_info::type_data::instance() noexcept
	{
		constinit static auto value = type_data{type_selector<T>};
		return &value;
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
