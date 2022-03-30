//
// Created by switchblade on 2022-02-06.
//

#pragma once

#include <utility>

#include "../math/detail/util.hpp"
#include "aligned_storage.hpp"
#include "type_info.hpp"

namespace sek
{
	class any
	{
	public:
		/** Returns an `any` instance referencing the passed external object. */
		template<typename T>
		[[nodiscard]] constexpr static any make_ref(T &ptr) noexcept
		{
			return any{std::in_place_type<T &>, ptr};
		}

	private:
		enum flags_t
		{
			/** Stored value is a reference to some other object (no memory management is needed). */
			EXTERNAL_REF = 0b1,
			/** Stored value is const (can only return const pointers). */
			CONST_OBJECT = 0b10,
			/** Value is stored locally (it is trivially copyable & fits into local data). */
			LOCAL_STORAGE = 0b100,
		};

		template<typename T>
		constexpr static bool is_local_candidate = std::is_trivially_copyable_v<T> && sizeof(T) <= sizeof(void *);

		struct vtable_t
		{
			template<typename T>
			constexpr static vtable_t bind() noexcept
			{
				return vtable_t{
					.copy_func =
						[](any &dest, const void *src)
					{
						using U = std::add_const_t<T>;
						dest.template init_owned<T, U &>(*static_cast<U *>(src));
					},
					.delete_func =
						[](any &data)
					{
						if constexpr (!is_local_candidate<T>)
							delete *data.storage.template get<T *>();
						else
							std::destroy_at(data.storage.template get<T>());
					},
				};
			}
			template<typename T>
			constexpr static vtable_t bind() noexcept requires std::is_reference_v<T>
			{
				return vtable_t{
					.copy_func =
						[](any &dest, const void *src)
					{
						if constexpr (std::is_const_v<std::remove_reference_t<T>>)
							dest.flags = static_cast<flags_t>(dest.flags | CONST_OBJECT);
						dest.flags = static_cast<flags_t>(dest.flags | EXTERNAL_REF);
						*dest.storage.template get<const void *>() = src;
					},
					.delete_func = nullptr,
				};
			}

			void (*copy_func)(any &, const void *) = nullptr;
			void (*delete_func)(any &) = nullptr;
		};

		[[nodiscard]] constexpr static type_info add_const(type_info original) noexcept
		{
			if (original.is_const())
				return original;
			else
				return original.get_const_variant();
		}

	public:
		constexpr any() noexcept = default;

		/** Initializes `any` as a reference to an external object. */
		template<not_same<any> T>
		constexpr any(std::in_place_type_t<T &>, T &ref)
			: value_type(type_info::get<T>()), vtable(vtable_t::bind<T &>())
		{
			init_ref(std::addressof(ref));
		}

		/** Initializes `any` by constructing the specified type in-place. */
		template<typename T, typename... Args>
		constexpr any(std::in_place_type_t<T>, Args &&...args)
			: value_type(type_info::get<T>()), vtable(vtable_t::bind<T>())
		{
			init_owned<T, Args...>(std::forward<Args>(args)...);
		}
		/** Initializes `any` by-copy from the passed object. */
		template<typename T>
		constexpr any(const T &value) requires not_same<std::decay_t<T>, any> : any(std::in_place_type<T>, value)
		{
		}
		/** Initializes `any` by-move from the passed object. */
		template<typename T>
		constexpr any(T &&value) requires not_same<std::decay_t<T>, any>
			: any(std::in_place_type<std::remove_reference_t<T>>, std::forward<T>(value))
		{
		}

		/** Initializes `any` from a type and data pointer. */
		constexpr any(type_info type, void *data) noexcept : value_type(type) { init_ref(data); }
		/** Initializes `any` from a type and const data pointer. */
		constexpr any(type_info type, const void *data) noexcept : value_type(add_const(type)) { init_ref(data); }

		/** Makes a copy of the value stored inside the passed `any` object. */
		constexpr any(const any &other) : value_type(other.value_type), vtable(other.vtable)
		{
			if (vtable.copy_func) [[likely]]
				vtable.copy_func(*this, other.const_data());
		}
		constexpr any(any &&other) noexcept { swap(other); }
		/** Destroys the old value & makes copy of the value stored inside the passed `any` object. */
		constexpr any &operator=(const any &other)
		{
			if (this != &other)
			{
				destroy();
				flags = {};
				value_type = other.value_type;
				vtable = other.vtable;
				if (vtable.copy_func) [[likely]]
					vtable.copy_func(*this, other.const_data());
			}
			return *this;
		}
		constexpr any &operator=(any &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr ~any() { destroy(); }

		/** Checks if the `any` instance contains a value. */
		[[nodiscard]] constexpr bool empty() const noexcept { return value_type.empty(); }
		/** Checks if the `any` instance references an external object. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return flags & EXTERNAL_REF; }
		/** Checks if the contained value is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return flags & CONST_OBJECT; }

		/** Returns the type of the contained value. If there is no value stored, returns an empty `type_info`. */
		[[nodiscard]] constexpr type_info type() const noexcept { return value_type; }
		/** Returns the type id of the contained value.
		 * @note `any` must contain a value. */
		[[nodiscard]] constexpr type_id tid() const noexcept { return type().tid(); }

		/** Checks if the stored value is of specific type. */
		[[nodiscard]] constexpr bool contains(type_id id) const noexcept { return !empty() && type().tid() == id; }
		/** @copydoc contains */
		[[nodiscard]] constexpr bool contains(type_info t) const noexcept { return !empty() && type() == t; }
		/** @copydoc contains */
		template<typename T>
		[[nodiscard]] constexpr bool contains() const noexcept
		{
			return contains(type_id::identify<T>());
		}

		/** Returns pointer to the stored value's data.
		 * @note If the value is const-qualified, returns nullptr. */
		[[nodiscard]] constexpr void *data() noexcept
		{
			if (is_const()) [[unlikely]]
				return nullptr;
			return data_impl();
		}
		/** Returns constant pointer to the stored value's data. */
		[[nodiscard]] constexpr const void *const_data() const noexcept { return data_impl(); }
		/** @copydoc const_data */
		[[nodiscard]] constexpr const void *data() const noexcept { return const_data(); }

		/** Returns pointer to the stored value cast to the target type.
		 * @tparam T Type to cast the stored value to.
		 * @throw bad_type_exception If `T` is incompatible with the stored type.
		 * @note If the stored value is const-qualified while the destination type is not, returns nullptr. */
		template<typename T>
		[[nodiscard]] constexpr T *as()
		{
			assert_compatible<T>();
			if constexpr (std::is_const_v<T>)
				return static_cast<T *>(const_data());
			else
				return static_cast<T *>(data());
		}
		/** Returns const pointer to the stored value cast to the target type.
		 * @tparam T Type to cast the stored value to.
		 * @throw bad_type_exception If `T` is incompatible with the stored type. */
		template<typename T>
		[[nodiscard]] constexpr const T *as() const
		{
			assert_compatible<T>();
			return static_cast<const T *>(const_data());
		}

		/** Returns an `any` instance referencing the stored value. */
		[[nodiscard]] constexpr any to_ref() noexcept
		{
			if (is_const()) [[unlikely]]
				return any{type(), const_data()};
			else
				return any{type(), data_impl()};
		}
		/** @copydoc to_ref */
		[[nodiscard]] constexpr any to_ref() const noexcept { return any{type(), data_impl()}; }

		/** Resets `any` to an empty state, destroying the stored value if necessary. */
		constexpr void reset()
		{
			destroy();
			value_type = {};
			vtable = {};
			flags = {};
		}

		constexpr void swap(any &other) noexcept
		{
			using std::swap;
			swap(storage, other.storage);
			swap(value_type, other.value_type);
			swap(vtable, other.vtable);
			swap(flags, other.flags);
		}

		friend constexpr void swap(any &a, any &b) noexcept { a.swap(b); }

	private:
		template<typename T>
		constexpr void assert_compatible() const
		{
			if (!value_type.template compatible_with<T>()) [[unlikely]]
				throw bad_type_exception(type_id::identify<T>());
		}
		[[nodiscard]] constexpr bool is_local() const noexcept { return flags & LOCAL_STORAGE; }

		[[nodiscard]] constexpr void *external_ptr() noexcept { return *storage.get<void *>(); }
		[[nodiscard]] constexpr const void *external_ptr() const noexcept { return *storage.get<const void *>(); }
		[[nodiscard]] constexpr void *data_impl() noexcept
		{
			return is_local() ? static_cast<void *>(storage.data()) : external_ptr();
		}
		[[nodiscard]] constexpr const void *data_impl() const noexcept
		{
			return is_local() ? static_cast<const void *>(storage.data()) : external_ptr();
		}

		constexpr void destroy()
		{
			if (vtable.delete_func) [[likely]]
				vtable.delete_func(*this);
		}

		template<typename T>
		constexpr void init_ref(T *ptr) noexcept
		{
			flags = static_cast<flags_t>(flags | EXTERNAL_REF);
			if constexpr (std::is_const_v<T>)
			{
				flags = static_cast<flags_t>(flags | CONST_OBJECT);
				*storage.get<const void *>() = static_cast<const void *>(ptr);
			}
			else
				*storage.get<void *>() = static_cast<void *>(ptr);
		}
		template<typename T, typename... Args>
		constexpr void init_owned(Args &&...args) requires std::constructible_from<T, Args...>
		{
			if constexpr (is_local_candidate<T>)
			{
				flags = static_cast<flags_t>(flags | LOCAL_STORAGE);
				std::construct_at(storage.template get<T>(), std::forward<Args>(args)...);
			}
			else
				*storage.template get<T *>() = new T{std::forward<Args>(args)...};

			if constexpr (std::is_const_v<T>) flags = static_cast<flags_t>(flags | CONST_OBJECT);
		}

		type_storage<void *> storage = {};
		type_info value_type = {};
		vtable_t vtable = {};
		flags_t flags = {};
	};

	template<forward_iterator_for<any> Iter>
	constexpr void type_info::constructor_info::invoke(void *ptr, Iter args_begin, Iter args_end) const
	{
		auto sign = signature();
		auto args_array = new const void *[sign.size()];
		try
		{
			/* Verify signature & fill the array. */
			size_t i = 0;
			for (auto expected = sign.begin(); expected != sign.end() && args_begin != args_end; ++expected, ++args_begin, ++i)
			{
				decltype(auto) arg = *args_begin;
				if (arg.empty() || *expected != arg.type()) [[unlikely]]
					throw bad_type_exception("Invalid constructor argument");

				args_array[i] = arg.const_data();
			}
			if (i != sign.size()) [[unlikely]]
				throw bad_type_exception("Passed argument sequence is too short");

			/* Arguments array is complete now. */
			invoke(ptr, args_array);
			delete[] args_array;
		}
		catch (...)
		{
			delete[] args_array;
			throw;
		}
	}

	constexpr any type_info::attribute_info::get() const noexcept { return any{type(), data()}; }
}	 // namespace sek