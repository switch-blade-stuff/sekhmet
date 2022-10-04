//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "type_info.hpp"

namespace sek
{
	class object;

	template<>
	[[nodiscard]] constexpr std::string_view type_name<object>() noexcept
	{
		return "sek::object";
	}

	/** @brief Base interface used to attach runtime type information to a polymorphic inheritance tree. */
	class SEK_API object
	{
		friend constexpr type_info type_of(const object &) noexcept;

		// clang-format off
		template<typename T, typename U>
		friend T object_cast(U &&) noexcept requires std::is_reference_v<T> && std::is_reference_v<U>;
		template<typename T, typename U>
		friend T object_cast(U &&) noexcept requires std::is_pointer_v<T> && std::is_pointer_v<U>;
		// clang-format on

		enum class cast_status : int
		{
			SAME_TYPE,	/* `from` is same as `to`. */
			BASE_TYPE,	/* `from` is base of `to`. */
			CHILD_TYPE, /* `from` is child of `to`. */
			UNRELATED,	/* `from` and `to` are not directly related. */
		};

		static cast_status check_cast(type_info from, type_info to) noexcept;
		static const object *checked_ptr_cast(const object *ptr, type_info from, type_info to) noexcept;
		static const object &checked_ref_cast(const object &ptr, type_info from, type_info to);

	public:
		constexpr object() noexcept = default;
		virtual ~object() = 0;

	protected:
		/** Returns the actual `type_info` of the type. Must be specialized by children. */
		[[nodiscard]] virtual type_info get_type_info() const noexcept = 0;
	};

	/** Returns the actual type of an `object`. */
	[[nodiscard]] constexpr type_info type_of(const object &obj) noexcept { return obj.get_type_info(); }

	// clang-format off
	/** Preforms a safe dynamic cast between references of `object` types.
	 * @return Reference to `obj` dynamically casted to `T`.
	 * @note Both types must be part of the same inheritance tree.
	 * @throw type_error If types are not part of the same inheritance tree. */
	template<typename T, typename U>
	[[nodiscard]] T object_cast(U &&obj) noexcept requires std::is_reference_v<T> && std::is_reference_v<U>
	{
		using T_decay = std::remove_cvref_t<T>;
		using U_decay = std::remove_cvref_t<U>;

		static_assert(is_preserving_cv_cast_v<std::remove_reference_t<U>, std::remove_reference_t<T>>,
					  "Cannot cast away qualifiers");
		static_assert(std::is_base_of_v<object, T_decay> && std::is_base_of_v<object, U_decay>,
					  "Can only cast between children of `object`");

		if constexpr (std::is_base_of_v<T_decay, U_decay>)
			return static_cast<T>(obj);
		else
		{
			const auto to_type = type_info::get<T>(obj);
			const auto from_type = type_of(obj);
			auto &src_ref = const_cast<const U_decay &>(obj);
			auto &obj_ref = object::checked_ref_cast(src_ref, from_type, to_type);
			auto &dst_ref = static_cast<const T_decay &>(obj_ref);
			return const_cast<T>(dst_ref);
		}
	}
	/** Preforms a safe dynamic cast between pointers of `object` types.
	 * @return Pointer to `obj` dynamically casted to `T`, or `nullptr` if a cast is not possible.
	 * @note Both types must be part of the same inheritance tree. */
	template<typename T, typename U>
	[[nodiscard]] T object_cast(U &&obj) noexcept requires std::is_pointer_v<T> && std::is_pointer_v<U>
	{
		using T_decay = std::remove_cvref_t<std::remove_pointer_t<T>>;
		using U_decay = std::remove_cvref_t<std::remove_pointer_t<U>>;

		static_assert(is_preserving_cv_cast_v<std::remove_pointer_t<U>, std::remove_pointer_t<T>>,
					  "Cannot cast away qualifiers");
		static_assert(std::is_base_of_v<object, T_decay> && std::is_base_of_v<object, U_decay>,
					  "Can only cast between children of `object`");

		if constexpr (std::is_base_of_v<T_decay, U_decay>)
			return static_cast<T>(obj);
		else
		{
			const auto to_type = type_info::get<T>(obj);
			const auto from_type = type_of(obj);
			auto *src_ptr = const_cast<const U_decay *>(obj);
			auto *obj_ptr = object::checked_ptr_cast(src_ptr, from_type, to_type);
			auto *dst_ptr = static_cast<const T_decay *>(obj_ptr);
			return const_cast<T>(dst_ptr);
		}
	}
	// clang-format on
}	 // namespace sek

/** @brief Creates required protected member definitions for a type that derives from `sek::object`. */
#define SEK_DEFINE_OBJECT()                                                                                            \
protected:                                                                                                             \
	[[nodiscard]] virtual type_info get_type_info() const noexcept override                                            \
	{                                                                                                                  \
		return type_info::get<std::remove_cvref_t<decltype(*this)>>();                                                 \
	}                                                                                                                  \
                                                                                                                       \
private: