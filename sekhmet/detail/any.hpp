//
// Created by switchblade on 2022-02-06.
//

#pragma once

#include <utility>

#include "../math/detail/util.hpp"
#include "aligned_storage.hpp"
#include "alloc_util.hpp"
#include "type_info.hpp"

namespace sek::detail
{
	class any_base
	{
	protected:
		constexpr explicit any_base(type_info info) noexcept : info(info) {}
		template<typename T>
		constexpr explicit any_base(type_selector_t<T>) noexcept : any_base(type_info::get<T>())
		{
		}

	public:
		constexpr any_base() noexcept = default;

		/** Returns type info of the underlying type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return info; }
		/** Checks if the underlying object is present. */
		[[nodiscard]] constexpr bool empty() const noexcept { return !type().valid(); }

		/** Checks if the underlying object is of type T. */
		template<typename T>
		[[nodiscard]] bool contains() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return !empty() && info.tid() == id;
		}
		/** Returns type id of the underlying type. */
		[[nodiscard]] type_id tid() const noexcept { return info.tid(); }

	protected:
		template<typename T>
		constexpr void assert_valid_cast() const
		{
			if (empty()) [[unlikely]]
				throw bad_type_exception("Unable to cast empty any instance");
			else if (!info.template is_compatible<T>()) [[unlikely]]
				throw bad_type_exception(tid(), type_id::identify<T>());
		}

		constexpr void swap(any_base &other) noexcept { std::swap(info, other.info); }

		/** Type info of the stored object. */
		type_info info = {};
	};

	/** @brief Structure used to store type-checked value of any type.
	 *
	 * @note Trivially-copyable types with size less than size of a pointer and alignment less than alignment of a
	 * pointer will be stored in-place. Other types require dynamic memory allocation. */
	class any : public any_base
	{
	private:
		struct vtable_t
		{
			template<typename T>
			constexpr static vtable_t bind() noexcept
			{
				if constexpr (local_storage<T>)
					return {};
				else
					return {
						+[](void *&dest, const void *src) -> void
						{
							if constexpr (std::is_copy_constructible_v<T>)
								dest = std::construct_at(allocator<T>{}.allocate(1), *static_cast<const T *>(src));
							else
								throw bad_type_exception("Stored type is not copy-constructible");
						},
						+[](void *ptr) -> void
						{
							auto *value = static_cast<T *>(ptr);
							std::destroy_at(value);
							allocator<T>{}.deallocate(value, 1);
						},
					};
			}

			constexpr void copy(void *&dest, const void *src) const
			{
				if (copy_func) copy_func(dest, src);
			}
			constexpr void destroy(void *value) const
			{
				if (destroy_func) destroy_func(value);
			}

			void (*copy_func)(void *&, const void *) = nullptr;
			void (*destroy_func)(void *) = nullptr;
		};

		using local_t = aligned_storage<sizeof(void *), alignof(void *)>;

		constexpr static auto max_align = max(alignof(local_t), alignof(vtable_t), alignof(type_info));
		constexpr static auto max_size = sizeof(local_t);

		template<typename T>
		constexpr static auto local_storage = std::is_trivially_copyable_v<T> && sizeof(T) <= max_size && alignof(T) <= max_align;

	private:
		template<typename T>
		constexpr explicit any(type_selector_t<T> s) noexcept : any_base(s), vtable(vtable_t::bind<T>())
		{
		}

	public:
		/** Constructs an empty `any` instance. */
		constexpr any() noexcept = default;
		/** Destroys the underlying object. */
		~any() { vtable.destroy(external_data); }

		/** Copies contents of another `any` instance.
		 * @param other Any object to copy content from.
		 * @throw bad_type_exception If the underlying object is not copyable. */
		any(const any &other) : any_base(other), local_data(other.local_data), vtable(other.vtable)
		{
			vtable.copy(external_data, other.external_data);
		}
		/** Takes ownership of contents of another `any` instance.
		 * @param other Any object to move content from. */
		any(any &&other) noexcept
			: any_base(std::move(other)),
			  local_data(std::exchange(other.local_data, {})),
			  vtable(std::exchange(other.vtable, {}))
		{
		}

		/** Copy-assigns the `any` instance by destroying the current contents & copying other's content.
		 * @param other Any object to copy content from. */
		any &operator=(const any &other)
		{
			if (this != &other) any{other}.swap(*this);
			return *this;
		}
		/** Move-assigns the `any` instance by destroying the current contents & taking ownership of other's content.
		 * The other `any` instance is then empty.
		 * @param other Any object to move content from. */
		any &operator=(any &&other) noexcept
		{
			any{std::forward<any>(other)}.swap(*this);
			return *this;
		}

		/** Constructs an instance of underlying type in-place.
		 * @param args Arguments used to construct the instance. */
		template<typename T, typename... Args, typename U = std::decay_t<T>>
		explicit any(std::in_place_type_t<T>, Args &&...args) : any(type_selector<U>)
		{
			U *dest;
			if constexpr (local_storage<U>)
				dest = local_data.get<U>();
			else
				external_data = dest = allocator<U>{}.allocate(1);
			std::construct_at(dest, std::forward<Args>(args)...);
		}
		/** Copy-constructs an instance of underlying type.
		 * @param value Value to copy. */
		template<typename T>
		any(const T &value) : any(std::in_place_type<T>, value)
		{
		}
		/** Move-constructs an instance of underlying type.
		 * @param value Value to move. */
		template<typename T>
		any(T &&value) : any(std::in_place_type<T>, std::forward<T>(value))
		{
		}

		/** Returns reference to the underlying object as T reference.
		 * @return `T &` reference to the underlying object.
		 * @throw bad_any_cast If the underlying object is not compatible with T. */
		template<typename T>
		[[nodiscard]] T &as()
		{
			assert_valid_cast<T>();
			return *static_cast<T *>(data());
		}
		/** Returns reference to the underlying object as T reference.
		 * @return `const T &` reference to the underlying object.
		 * @throw bad_any_cast If the underlying object is not compatible with T. */
		template<typename T>
		[[nodiscard]] const T &as() const
		{
			assert_valid_cast<T>();
			return *static_cast<const T *>(data());
		}

		/** Returns pointer to the raw data of the underlying object. */
		[[nodiscard]] constexpr void *data() noexcept { return is_local() ? local_data.data() : external_data; }
		/** @copydoc data */
		[[nodiscard]] constexpr const void *data() const noexcept
		{
			return is_local() ? local_data.data() : external_data;
		}

		constexpr void swap(any &other) noexcept
		{
			any_base::swap(other);

			using std::swap;
			swap(local_data, other.local_data);
			swap(vtable, other.vtable);
		}

		friend constexpr void swap(any &a, any &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr bool is_local() const noexcept { return vtable.copy_func == nullptr; }

		union
		{
			/** Data of the object stored locally. */
			local_t local_data = {};
			/** Pointer to the referenced object. */
			void *external_data;
		};

		/** Vtable containing bounded copy constructor & destructor for the type. */
		vtable_t vtable = {};
	};

	/** @brief Type-checked reference of any type. */
	class any_ref : public any_base
	{
	public:
		/** Constructs an empty `any_ref` instance. */
		constexpr any_ref() noexcept = default;

		/** Constructs `any_ref` from a type info and a type-erased pointer.
		 * @param type Type of the referenced object.
		 * @param data Pointer to the object's data. */
		constexpr any_ref(type_info type, void *data) noexcept : any_base(type), data(data) {}
		/** @copydoc any_ref */
		constexpr any_ref(type_info type, const void *data) noexcept : any_base(type), data(const_cast<void *>(data)) {}

		/** Initializes `any_ref` to reference the passed object.
		 * @param ref Object to reference.
		 * @note This constructor participates in overload only if `std::decay_t<T>` is not `any_ref` or `any`. */
		template<typename T, typename U = std::decay_t<T>>
		constexpr any_ref(T &value) noexcept requires(!std::same_as<U, any_ref> && !std::same_as<U, any>)
			: any_base(type_selector<T>), data((void *) &value)
		{
		}
		/** Initializes `any_ref` to reference object contained by an `any` instance.
		 * @param a `any` instance containing an object to reference. */
		constexpr any_ref(const any &a) noexcept : any_ref(a.type(), a.data()) {}

		/** Uses the stored type info to construct the referenced object.
		 * @param args Arguments passed to the constructor. */
		template<typename... Args>
		void construct(Args... args) const
		{
			info.template construct<Args...>(data, std::forward<Args>(args)...);
		}
		/** Uses the stored type info to destroy the referenced object.
		 * @note Does not reset the `any_ref` instance. */
		void destroy() const { info.destroy(data); }

		/** Resets the `any_ref` instance to an empty state. */
		constexpr void reset() noexcept
		{
			info = type_info{};
			data = nullptr;
		}

		/** Returns reference to the underlying object as T reference.
		 * @return Reference to the underlying object.
		 * @throw bad_any_cast If the underlying object is not compatible with T or `any_ref` is empty. */
		template<typename T>
		[[nodiscard]] T &as() const
		{
			any_base::assert_valid_cast<T>();
			return *static_cast<T *>(data);
		}

		constexpr void swap(any_ref &other) noexcept
		{
			any_base::swap(other);
			std::swap(data, other.data);
		}

		friend constexpr void swap(any_ref &a, any_ref &b) noexcept { a.swap(b); }

	private:
		void *data = nullptr;
	};

	constexpr any_ref type_info::attribute_iterator::operator*() const noexcept
	{
		return {type_info{node->type}, node->data};
	}
	inline any_ref type_info::get_attribute(type_id id) const noexcept
	{
		for (auto attr = data->attributes; attr != nullptr; attr = attr->next)
			if (attr->type->tid == id) return {type_info{attr->type}, attr->data};
		return {};
	}
}	 // namespace sek::detail