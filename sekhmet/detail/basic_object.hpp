//
// Created by switchblade on 2022-04-01.
//

#pragma once

#include <bit>

#include "meta_containers.hpp"
#include "type_id.hpp"

namespace sek
{
	namespace detail
	{
		struct object_data
		{
			struct attribute
			{
				template<typename T>
				constexpr explicit attribute(const T *data) noexcept
					: name(type_name<T>()), data(static_cast<const void *>(data))
				{
				}

				meta_view<char> name;
				const void *data;
			};

			template<auto... Vs>
			[[nodiscard]] constexpr static meta_view<attribute> extract_attributes() noexcept
			{
				if constexpr (sizeof...(Vs) != 0)
				{
					constexpr auto &values = filter_array_constant<attribute, Vs...>::value;
					return meta_view<attribute>{values};
				}
				else
					return {};
			}
			template<auto... Vs>
			[[nodiscard]] constexpr static meta_view<meta_view<char>> extract_parents() noexcept
			{
				if constexpr (sizeof...(Vs) != 0)
				{
					constexpr auto &values = filter_array_constant<meta_view<char>, Vs...>::value;
					return meta_view<meta_view<char>>{values};
				}
				else
					return {};
			}

			type_id tid;
			meta_view<meta_view<char>> parents;
			meta_view<attribute> attributes;
		};
	}	 // namespace detail

	/** @brief Base type that enables it's children to use runtime type information.
	 * @note `basic_object` can not participate in multiple or `virtual` inheritance. */
	struct basic_object
	{
		constexpr virtual ~basic_object() = 0;

		/** Returns type id of this object. */
		[[nodiscard]] type_id tid() const noexcept { return internal_get_object_data_instance().tid; }

		/** Returns true if the object inherits from the specified type. */
		[[nodiscard]] bool inherits(type_id id) const noexcept
		{
			auto &data = internal_get_object_data_instance();
			return std::any_of(data.parents.begin(),
							   data.parents.end(),
							   [name = id.name()](auto p) noexcept -> bool
							   { return std::equal(p.begin(), p.end(), name.begin(), name.end()); });
		}
		/** Returns true if the object inherits from `T`. */
		template<typename T>
		[[nodiscard]] bool inherits() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return inherits(id);
		}

		/** Returns true if the object has an attribute of a specific type. */
		[[nodiscard]] bool has_attribute(type_id id) const noexcept { return get_attribute(id) != nullptr; }
		/** Returns true if the object has an attribute of type `T`. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return get_attribute(id);
		}
		/** If the type has an attribute of type `T`, returns that attribute, otherwise, returns nullptr. */
		template<typename T>
		[[nodiscard]] const T *get_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return static_cast<const T *>(get_attribute(id));
		}

	protected:
		/** For internal use only! */
		[[nodiscard]] virtual const detail::object_data &internal_get_object_data_instance() const noexcept = 0;

	private:
		[[nodiscard]] const void *get_attribute(type_id id) const noexcept
		{
			auto name = id.name();
			for (auto &attr : internal_get_object_data_instance().attributes)
				if (std::equal(attr.name.begin(), attr.name.end(), name.begin(), name.end())) return attr.data;
			return nullptr;
		}
	};
	constexpr basic_object::~basic_object() = default;

	template<typename T>
	concept is_object_type = std::is_base_of_v<basic_object, T>;

	/** @brief Casts `basic_object` from the `From` type to the `To` type.
	 *
	 * Object cast from reference of object type `A` (`a`) to object type `B` (`b`) follows these rules:
	 * - Otherwise, if `std::decay_t<B>` is the same as `std::decay_t<A>`, equivalent to `static_cast<decltype(b)>(a)`.
	 * - Otherwise, if `std::decay_t<B>` is a parent of `std::decay_t<A>`, equivalent to `static_cast<decltype(b)>(a)`.
	 * - Otherwise, if `std::decay_t<B>` is a parent of the actual type of `a`, returns a pointer/reference to `B`
	 * referencing the object stored at `a`.
	 * - Otherwise, the cast is invalid.
	 *
	 * @param ptr Pointer to the `From` object.
	 * @return `ptr` casted to the `To` type, or nullptr if such cast is not possible.
	 * @note `object_cast` can not be used to cast away const-ness or volatility. Use `const_cast` instead. */
	template<is_object_type To, is_object_type From>
	constexpr To *object_cast(From *ptr) noexcept requires(is_preserving_cv_cast_v<From, To>)
	{
		if (!ptr) [[unlikely]]
			return nullptr;

		using DecayFrom = std::decay_t<From>;
		using DecayTo = std::decay_t<To>;

		if constexpr (std::same_as<DecayFrom, DecayTo> || std::is_base_of_v<DecayTo, DecayFrom>)
			return static_cast<To *>(ptr);
		else
		{
			auto from = static_cast<transfer_cv_t<From, basic_object> *>(ptr);
			auto to = from->template inherits<DecayTo>() ? static_cast<transfer_cv_t<To, basic_object> *>(from) : nullptr;
			return static_cast<To *>(to);
		}
	}
	/** @copybrief object_cast
	 * @copydetails object_cast
	 *
	 * @param ref Reference to the `From` object.
	 * @return `ref` casted to the `To` type.
	 * @throw `std::bad_cast` If such cast is not possible.
	 * @note `object_cast` can not be used to cast away const-ness or volatility. Use `const_cast` instead. */
	template<is_object_type To, is_object_type From>
	constexpr To &object_cast(From &ptr) noexcept
	{
		if (auto result = object_cast<To>(std::addressof(ptr)); !result) [[unlikely]]
			throw std::bad_cast();
		else
			return *result;
	}
}	 // namespace sek

/** Macro used to define an object's parent type for use with `SEK_OBJECT_TYPE`. */
#define SEK_OBJECT_PARENT(T) sek::meta_view<char>(sek::type_name<T>())
/** Macro used to define an object's attribute for use with `SEK_OBJECT_TYPE`. */
#define SEK_OBJECT_ATTRIBUTE(V) sek::detail::object_data::attribute(&sek::auto_constant<V>::value)

/** Macro used to generate object type's body.
 * Optionally, accepts any amount of `SEK_OBJECT_PARENT` & `SEK_OBJECT_ATTRIBUTE` arguments. */
#define SEK_OBJECT_TYPE(...)                                                                                           \
protected:                                                                                                             \
	virtual const sek::detail::object_data &internal_get_object_data_instance() const noexcept override                \
	{                                                                                                                  \
		constinit static const auto value = []() noexcept -> sek::detail::object_data                                  \
		{                                                                                                              \
			return sek::detail::object_data{                                                                           \
				.tid = type_id::identify<std::decay_t<decltype(*this)>>(),                                             \
				.parents = sek::detail::object_data::extract_parents<__VA_ARGS__>(),                                   \
				.attributes = sek::detail::object_data::extract_attributes<__VA_ARGS__>(),                             \
			};                                                                                                         \
		}();                                                                                                           \
		return value;                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
private:
