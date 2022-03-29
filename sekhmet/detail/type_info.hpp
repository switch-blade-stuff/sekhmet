//
// Created by switchblade on 2022-02-04.
//

#pragma once

#include <cstdarg>
#include <ranges>
#include <vector>

#include "assert.hpp"
#include "engine_exception.hpp"
#include "type_data.hpp"

namespace sek
{
	class any;
	class any_ref;

	/** @brief Exception thrown when a type is invalid or incompatible with another. */
	class bad_type_exception : engine_exception
	{
	public:
		SEK_API bad_type_exception() noexcept = default;
		SEK_API explicit bad_type_exception(const char *msg) noexcept;
		SEK_API explicit bad_type_exception(type_id type) noexcept;
		SEK_API bad_type_exception(type_id from, type_id to) noexcept;
		SEK_API ~bad_type_exception() override;

		[[nodiscard]] const char *what() const noexcept override { return msg; }

	private:
		const char *msg = nullptr;
	};

	/** @brief Structure used to represent information about a type. */
	class type_info
	{
	public:
		/** Adds a type to runtime lookup database.
		 * @param type Type info of the type to add to runtime database.
		 * @return true if a type was added successfully, false otherwise.
		 * @note This function will fail if a type with the same id was already registered. */
		static bool register_type(type_info type);
		/** Invokes type factory for the type and adds it to runtime lookup database.
		 * @return true if a type was added successfully, false otherwise.
		 * @note This function will fail if a type with the same id was already registered. */
		template<typename T>
		static bool register_type()
		{
			return register_type(get<T>());
		}
		/** Removes a type from runtime lookup database.
		 * @param type Type info of the type to remove.
		 * @return true if a type was removed successfully, false otherwise. */
		static bool deregister_type(type_info type);
		/** Removes a type from runtime lookup database.
		 * @return true if a type was removed successfully, false otherwise. */
		template<typename T>
		static bool deregister_type()
		{
			return deregister_type(get<T>());
		}

		/** @brief RAII structure used to automatically register & deregister types to/from the runtime lookup database. */
		template<typename T>
		struct type_guard
		{
			type_guard(const type_guard &) = delete;
			type_guard &operator=(const type_guard &) = delete;
			type_guard(type_guard &&) = delete;
			type_guard &operator=(type_guard &&) = delete;

			type_guard() : added(register_type<T>()) {}
			~type_guard()
			{
				if (added) deregister_type<T>();
			}

			bool added;
		};

		/** Returns an instance of type info generated at compile time.
		 * @note Type is not required to be registered, */
		template<typename T>
		[[nodiscard]] constexpr static type_info get() noexcept
		{
			return type_info{&detail::type_data::instance<T>::value};
		}
		/** Looks up a type within the runtime lookup database.
		 * @tparam tid Id of the type to search for.
		 * @return `type_info` instance for the requested type. If an invalid tid was specified, returns an invalid type info.
		 * @note Type must be registered for it to be available. */
		[[nodiscard]] static type_info get(type_id tid) noexcept;
		/** Returns vector containing all currently registered types. */
		[[nodiscard]] static std::vector<type_info> all();

	private:
		constexpr explicit type_info(detail::type_data::handle data) noexcept : data(data) {}

		template<std::forward_iterator Iter>
		requires std::same_as<std::iter_value_t<Iter>, detail::type_data>
		struct type_info_extractor
		{
			typedef typename std::iterator_traits<Iter>::difference_type difference_type;
			typedef type_info value_type;
			typedef type_info reference;
			typedef std::forward_iterator_tag iterator_category;

			type_info_extractor() = delete;
			constexpr explicit type_info_extractor(Iter value) : value(value) {}

			constexpr type_info_extractor operator++(int) noexcept { return type_info_extractor{value++}; }
			constexpr type_info_extractor &operator++() noexcept
			{
				++value;
				return *this;
			}

			[[nodiscard]] constexpr type_info operator*() const noexcept { return type_info{*value}; }

			Iter value;
		};

	public:
		constexpr type_info() noexcept = default;

		/** Checks if the type info is valid. */
		[[nodiscard]] constexpr bool valid() const noexcept { return !data.empty(); }

		/** Returns id of the type. */
		[[nodiscard]] type_id tid() const noexcept { return data->tid; }
		/** Returns name of the type. */
		[[nodiscard]] std::string_view name() const noexcept { return data->tid.name(); }
		/** Returns hash of the type. */
		[[nodiscard]] std::size_t hash() const noexcept { return data->tid.hash(); }

		/** Returns the type's size. */
		[[nodiscard]] std::size_t size() const noexcept { return data->size; }
		/** Returns the type's alignment. */
		[[nodiscard]] std::size_t alignment() const noexcept { return data->alignment; }

		/** Checks if the type is const-qualified. */
		[[nodiscard]] bool is_const() const noexcept { return data->variant_type & detail::type_data::VARIANT_CONST; }
		/** Checks if the type is volatile-qualified. */
		[[nodiscard]] bool is_volatile() const noexcept
		{
			return data->variant_type & detail::type_data::VARIANT_VOLATILE;
		}
		/** Checks if the type is cv-qualified. */
		[[nodiscard]] bool is_cv() const noexcept
		{
			return data->variant_type == detail::type_data::VARIANT_CONST_VOLATILE;
		}

		/** Checks if the type is a qualified variant of another type.
		 * @return true if the type is a qualified variant, false otherwise. */
		[[nodiscard]] bool is_variant() const noexcept
		{
			return !data->variants[detail::type_data::VARIANT_PARENT].empty();
		}
		/** If the type is a qualified variant, returns the unqualified "parent" type.
		 * Otherwise, returns an empty type info. */
		[[nodiscard]] type_info get_variant_parent() const noexcept
		{
			return type_info{data->variants[detail::type_data::VARIANT_PARENT]};
		}

		/** Checks if the type has a const-qualified variant.
		 * @return true if the type has a const-qualified variant, false otherwise.
		 * @note If the type itself is const-qualified, it does not have a const-qualified variant. */
		[[nodiscard]] bool has_const_variant() const noexcept
		{
			return !data->variants[detail::type_data::VARIANT_CONST].empty();
		}
		/** Returns type info of the const-qualified variant of the type.
		 * @return Type info of the const-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is const-qualified, it does not have a const-qualified variant. */
		[[nodiscard]] type_info get_const_variant() const noexcept
		{
			return type_info{data->variants[detail::type_data::VARIANT_CONST]};
		}

		/** Checks if the type has a volatile-qualified variant.
		 * @return true if the type has a volatile-qualified variant, false otherwise.
		 * @note If the type itself is volatile-qualified, it does not have a volatile-qualified variant. */
		[[nodiscard]] bool has_volatile_variant() const noexcept
		{
			return !data->variants[detail::type_data::VARIANT_VOLATILE].empty();
		}
		/** Returns type info of the volatile-qualified variant of the type.
		 * @return Type info of the volatile-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is volatile-qualified, it does not have a volatile-qualified variant. */
		[[nodiscard]] type_info get_volatile_variant() const noexcept
		{
			return type_info{data->variants[detail::type_data::VARIANT_VOLATILE]};
		}

		/** Checks if the type has a cv-qualified variant.
		 * @return true if the type has a cv-qualified variant, false otherwise.
		 * @note If the type itself is either const or volatile-qualified, it does not have a cv-qualified variant. */
		[[nodiscard]] bool has_cv_variant() const noexcept
		{
			return !data->variants[detail::type_data::VARIANT_CONST_VOLATILE].empty();
		}
		/** Returns type info of the cv-qualified variant of the type.
		 * @return Type info of the cv-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is either const or volatile-qualified, it does not have a cv-qualified variant. */
		[[nodiscard]] type_info get_cv_variant() const noexcept
		{
			return type_info{data->variants[detail::type_data::VARIANT_CONST_VOLATILE]};
		}

		/** Checks if the type has a variant of a specific type.
		 * @param id Id of the variant type.
		 * @return true if the type has a variant of `id` type, false otherwise. */
		[[nodiscard]] bool has_variant(type_id id) const noexcept
		{
			return (has_const_variant() && get_const_variant().tid() == id) ||
				   (has_volatile_variant() && get_volatile_variant().tid() == id) ||
				   (has_cv_variant() && get_cv_variant().tid() == id);
		}
		/** Checks if the type has a variant of a specific type.
		 * @tparam T Variant type.
		 * @return true if the type has a variant of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool has_variant() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return has_variant(id);
		}

		/** Checks if the type has a parent of a specific type.
		 * @param id Id of the parent type.
		 * @return true if the type has a parent of `id` type, false otherwise. */
		[[nodiscard]] bool has_parent(type_id id) const noexcept { return data->has_parent(id); }
		/** Checks if the type has a parent of a specific type.
		 * @tparam T Parent type.
		 * @return true if the type has a parent of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool has_parent() const noexcept
		{
			return data->template has_parent<T>();
		}

		/** Checks if the type is compatible with another type.
		 *
		 * Type `A` is considered compatible with type `B` if `A` == `B`, if `B` is a variant of `A`,
		 * or if `B` is a parent of `A`. If `A` is compatible with `B`,
		 * reference of type `A` can be implicitly cast to a reference of type `B`.
		 *
		 * @param id Id of the type to check for compatibility with.
		 * @return true if the type is compatible with `id` type, false otherwise. */
		[[nodiscard]] bool compatible_with(type_id id) const noexcept
		{
			return tid() == id || has_variant(id) || has_parent(id);
		}
		/** Checks if the type is compatible with a specific type.
		 *
		 * Type `A` is considered compatible with type `B` if `A` == `B`, if `B` is a variant of `A`,
		 * or if `B` is a parent of `A`. If `A` is compatible with `B`,
		 * reference of type `A` can be implicitly cast to a reference of type `B`.
		 *
		 * @tparam T Type to check for compatibility with.
		 * @return true if the type is compatible with `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool is_compatible() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return compatible_with(id);
		}

		/** Checks if the type has an attribute of a specific type.
		 * @param id Id of the attribute's type.
		 * @return true if the type has an attribute of `id` type, false otherwise. */
		[[nodiscard]] bool has_attribute(type_id id) const noexcept { return data->has_attribute(id); }
		/** Checks if the type has an attribute of a specific type.
		 * @tparam T Attribute's type.
		 * @return true if the type has an attribute of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept
		{
			return data->template has_attribute<T>();
		}
		/** Returns attribute of a specific type.
		 * @param id Id of the attribute's type.
		 * @return `any_ref` instance referencing the attribute if such attribute is present. Otherwise, and empty `any_ref` instance. */
		[[nodiscard]] any_ref get_attribute(type_id id) const noexcept;
		/** Returns attribute of a specific type.
		 * @tparam T Attribute's type.
		 * @return Pointer to attribute's data if such attribute is present, nullptr otherwise. */
		template<typename T>
		[[nodiscard]] const T *get_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			if (auto node = data->get_attribute(id); node != nullptr) [[likely]]
				return static_cast<T *>(node->data);
			else
				return nullptr;
		}

		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @param args_first Iterator to the start of the argument type sequence.
		 * @param args_first Iterator to the end of the argument type sequence.
		 * @return true if the type is constructible, false otherwise. */
		template<std::forward_iterator I>
		[[nodiscard]] bool constructible_with(I args_first, I args_last) const requires std::same_as<std::iter_value_t<I>, type_info>
		{
			return data->has_ctor(type_info_extractor{args_first}, type_info_extractor{args_last});
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @param args Range containing argument types.
		 * @return true if the type is constructible, false otherwise. */
		template<std::ranges::forward_range R>
		[[nodiscard]] bool constructible_with(const R &args) const
		{
			return constructible_with(std::ranges::begin(args), std::ranges::end(args));
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @tparam Args Arguments of the constructor.
		 * @return true if the type is constructible, false otherwise. */
		template<typename... Args>
		[[nodiscard]] bool constructible_with() const noexcept
		{
			return data->template has_ctor<Args...>();
		}

	private:
		/** Handle to type's type_data instance. */
		detail::type_data::handle data = {};
	};
}	 // namespace sek

/** Exports type info for a specific type.
 * This is needed in order to correctly link type factories & generated type data across translation units.
 *
 * @note It is generally not recommended to export qualified versions of a type (instead export the unqualified type).
 * @warning If a type is exported, a factory must be explicitly defined inside a single translation unit,
 * failing to do so will cause link errors.
 * @note Setting a custom type id via `SEK_SET_TYPE_ID` is not required, but is recommended.
 *
 * @example
 * ```cpp
 * // All translation units that include this header will link against the same type data & factory for `my_type`.
 * SEK_EXPORT_TYPE(my_type)
 * ``` */
#define SEK_EXPORT_TYPE(T)                                                                                             \
	template<>                                                                                                         \
	constexpr bool sek::detail::type_data::instance<T>::is_exported = true;                                            \
	extern template struct SEK_API_IMPORT sek::detail::type_data::instance<T>;

#define SEK_DECLARE_TYPE_2(T, name)                                                                                    \
	SEK_SET_TYPE_ID(T, name)                                                                                           \
	SEK_EXPORT_TYPE(T)
#define SEK_DECLARE_TYPE_1(T) SEK_DECLARE_TYPE_2(T, #T)

/** Declares a type with a custom id & exports it's type info.
 * Equivalent to using `SEK_SET_TYPE_ID` followed by `SEK_EXPORT_TYPE`.
 *
 * @note Type name is optional, if not specified will use a string made from the first argument.
 * @warning Every declared type must have a type factory defined inside a single translation unit (via
 * `SEK_TYPE_FACTORY`), a missing type factory will result in link errors.
 *
 * @example
 * ```cpp
 * // Type `some_namespace::some_type` will be identifiable as `my_type` and it's factory will be exported.
 * SEK_DECLARE_TYPE(some_namespace::some_type, "my_type")
 * ```
 * @example
 * ```cpp
 * // Type `some_other_type` will be identifiable as `some_other_type` and it's factory will be exported.
 * SEK_DECLARE_TYPE(some_other_type)
 * ```  */
#define SEK_DECLARE_TYPE(T, ...)                                                                                       \
	SEK_GET_MACRO_2(T, (__VA_ARGS__), SEK_DECLARE_TYPE_2, SEK_DECLARE_TYPE_1)(T, (__VA_ARGS__))

/* Creates a type factory for the specific type. Type factories are invoked on static initialization.
 *
 * @warning Types that have a factory must be exported via `SEK_EXPORT_TYPE`,
 * failing to do so may result in compilation/linking errors. */
#define SEK_TYPE_FACTORY(T)                                                                                            \
	static_assert(sek::detail::type_data::instance<T>::is_exported,                                                    \
				  "Type must be exported for type factory to work & be linked correctly");                             \
	template struct SEK_API_EXPORT sek::detail::type_data::instance<T>;                                                \
	namespace                                                                                                          \
	{                                                                                                                  \
		template<typename U>                                                                                           \
		struct sek_type_factory : sek::detail::type_factory_base<U>                                                    \
		{                                                                                                              \
			static void invoke() noexcept;                                                                             \
                                                                                                                       \
			static const sek_type_factory instance;                                                                    \
                                                                                                                       \
			sek_type_factory() noexcept { invoke(); }                                                                  \
		};                                                                                                             \
		template<typename U>                                                                                           \
		const sek_type_factory<U> sek_type_factory<U>::instance = {};                                                  \
                                                                                                                       \
		template<>                                                                                                     \
		void sek_type_factory<T>::invoke() noexcept;                                                                   \
		template struct sek_type_factory<T>;                                                                           \
	}                                                                                                                  \
	template<>                                                                                                         \
	void sek_type_factory<T>::invoke() noexcept
