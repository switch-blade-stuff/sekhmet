//
// Created by switchblade on 2022-04-01.
//

#pragma once

#include <bit>
#include <stdexcept>

#include "type_id.hpp"
#include "type_info.hpp"

namespace sek
{
	/** @brief Exception thrown on invalid object cast. */
	class object_cast_error : public std::runtime_error
	{
	public:
		object_cast_error() : std::runtime_error("Invalid object cast") {}
		~object_cast_error() override = default;
	};

	/** @brief Base type that enables it's children to use runtime type information.
	 * @note `basic_object` can not participate in multiple or `virtual` inheritance. */
	struct basic_object
	{
		constexpr virtual ~basic_object() = 0;

		/** Returns type info of this object. */
		[[nodiscard]] type_info type() const noexcept { return internal_get_type_info(); }
		/** Returns type id of this object. */
		[[nodiscard]] type_id tid() const noexcept { return type().tid(); }

	protected:
		/** @warning For internal use only! */
		[[nodiscard]] virtual type_info internal_get_type_info() const noexcept = 0;
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
			auto to = from->type().template inherits<DecayTo>() ? static_cast<transfer_cv_t<To, basic_object> *>(from) : nullptr;
			return static_cast<To *>(to);
		}
	}
	/** @copybrief object_cast
	 * @copydetails object_cast
	 *
	 * @param ref Reference to the `From` object.
	 * @return `ref` casted to the `To` type.
	 * @throw `object_cast_error` If such cast is not possible.
	 * @note `object_cast` can not be used to cast away const-ness or volatility. Use `const_cast` instead. */
	template<is_object_type To, is_object_type From>
	constexpr To &object_cast(From &ptr) noexcept
	{
		if (auto result = object_cast<To>(std::addressof(ptr)); !result) [[unlikely]]
			throw object_cast_error();
		else
			return *result;
	}
}	 // namespace sek

/** Macro used to initialize object type's body. */
#define SEK_OBJECT_BODY                                                                                                \
	::sek::type_info internal_get_type_info() const noexcept override { return type_info::get<decltype(*this)>(); }
