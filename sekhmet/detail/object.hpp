//
// Created by switchblade on 17/05/22.
//

#pragma once

#include "type_info.hpp"

namespace sek
{
	struct object;

	// clang-format off
	template<typename T>
	concept object_type = std::derived_from<std::remove_cv_t<T>, object>;
	template<typename To, typename From>
	concept valid_object_cast = object_type<From> && object_type<To>;
	// clang-format on

	/** @brief Exception thrown when an object cast is not possible. */
	class bad_object_cast : public std::runtime_error
	{
	public:
		bad_object_cast() : std::runtime_error("object_cast cannot cast between unrelated types") {}
		explicit bad_object_cast(const char *msg) : std::runtime_error(msg) {}
		~bad_object_cast() override = default;
	};

	/** @brief Base class for all object types. */
	struct SEK_API object
	{
		virtual ~object() = default;

		[[nodiscard]] virtual type_info type_of() const noexcept = 0;
	};

	/** Casts the `from` pointer to `ToPtr`. Supports up-casts and cross-casts.
	 * @tparam ToPtr Pointer type to cast `from` to.
	 * @param from Pointer to the object being cast.
	 * @return `from` cast to `ToPtr` or nullptr if such cast is not possible.
	 * @note Both types must be object types. */
	template<typename ToPtr, typename From, typename To = std::remove_cv_t<std::remove_pointer_t<ToPtr>>>
	[[nodiscard]] constexpr ToPtr object_cast(From *from) noexcept
		requires std::is_pointer_v<ToPtr> && valid_object_cast<To, From>
	{
		if constexpr (std::is_base_of_v<To, From>)
			return static_cast<ToPtr>(from);
		else if constexpr (std::is_base_of_v<From, To>)
		{
			auto *obj = static_cast<transfer_cv_t<From, object> *>(from);

			/* Check that the actual type of `from` is the same as or a child of `To`. */
			const auto to_type = type_info::get<To>();
			const auto from_type = obj->type_of();
			if (from_type == to_type || from_type.inherits(to_type.name())) [[likely]]
				return static_cast<ToPtr>(obj);
		}
		return nullptr;
	}
	/** Casts the `from` reference to `ToRef`. Supports up-casts and cross-casts.
	 * @tparam ToRef Reference type to cast `from` to.
	 * @param from Reference to the object being cast.
	 * @return `from` cast to `ToRef`.
	 * @throw bad_object_cast If a cast is not possible.
	 * @note Both types must be object types. */
	template<typename ToRef, typename From, typename To = std::remove_cv_t<std::remove_reference_t<ToRef>>>
	[[nodiscard]] constexpr ToRef &object_cast(From &from)
		requires std::is_reference_v<ToRef> && valid_object_cast<To, From>
	{
		auto *result = object_cast<std::remove_reference_t<ToRef> *>(std::addressof(from));
		if (!result) [[unlikely]]
			throw bad_object_cast();
		return *result;
	}
}	 // namespace sek

/** Helper macro used to initialize body of an object type `T`. */
#define SEK_OBJECT_BODY(T)                                                                                             \
	virtual sek::type_info type_of() const noexcept override                                                           \
	{                                                                                                                  \
		static_assert(std::is_base_of_v<sek::object, T>);                                                              \
		return sek::type_info::get<T>();                                                                               \
	}
