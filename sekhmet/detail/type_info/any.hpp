//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include <bit>
#include <memory>

#include "../../expected.hpp"
#include "type_data.hpp"
#include "type_error.hpp"

namespace sek
{
	namespace detail
	{
		class basic_any
		{
			template<typename T, typename U = std::remove_cvref_t<T>>
			constexpr static bool local_candidate = sizeof(U) <= sizeof(std::uintptr_t) && std::is_trivially_copyable_v<U>;

			struct storage_t
			{
				constexpr storage_t() noexcept = default;

				constexpr storage_t(const storage_t &) noexcept = default;
				constexpr storage_t &operator=(const storage_t &) noexcept = default;

				constexpr storage_t(storage_t &&other) noexcept
					: data(std::exchange(other.data, std::uintptr_t{})),
					  is_local(std::exchange(other.is_local, false)),
					  is_const(std::exchange(other.is_const, false))
				{
				}
				constexpr storage_t &operator=(storage_t &&other) noexcept
				{
					swap(other);
					return *this;
				}

				// clang-format off
				template<typename T>
				storage_t(T *ptr, bool is_const) noexcept : data(std::bit_cast<std::uintptr_t>(ptr)), is_ref(true), is_const(is_const)
				{
				}
				// clang-format on

				storage_t(std::in_place_type_t<void>, auto &&...) {}
				template<typename T, typename... Args>
				storage_t(std::in_place_type_t<T>, Args &&...args)
				{
					init<T>(std::forward<Args>(args)...);
					init_flags<T>();
				}

				template<typename T>
				constexpr void init_flags() noexcept
				{
					is_ref = std::is_lvalue_reference_v<T>;
					is_local = local_candidate<T>;
					is_const = std::is_const_v<std::remove_reference_t<T>>;
				}

				// clang-format off
				template<typename T, typename... Args>
				void init(Args &&...args)
				{
					if constexpr (sizeof...(Args) == 0 || std::is_aggregate_v<T>)
						data = std::bit_cast<std::uintptr_t>(new T{std::forward<Args>(args)...});
					else
						data = std::bit_cast<std::uintptr_t>(new T(std::forward<Args>(args)...));
				}
				template<typename T, typename... Args>
				void init(Args &&...args) requires local_candidate<T>
				{
					const auto ptr = std::bit_cast<T *>(&data);
					if constexpr (sizeof...(Args) == 0 || std::is_aggregate_v<T>)
						new (ptr) T{std::forward<Args>(args)...};
					else
						new (ptr) T(std::forward<Args>(args)...);
				}
				template<typename T, typename U>
				void init(U &ref) requires std::is_lvalue_reference_v<T>
				{
					data = std::bit_cast<std::uintptr_t>(std::addressof(ref));
				}

				template<typename T>
				void destroy()
				{
					if constexpr (std::is_bounded_array_v<T>)
						delete[] std::bit_cast<T *>(data);
					else
						delete std::bit_cast<T *>(data);
				}
				template<typename T>
				void destroy() requires local_candidate<T> { std::destroy_at(std::bit_cast<T *>(&data)); }
				template<typename T>
				void destroy() requires std::is_lvalue_reference_v<T> {}
				// clang-format on

				[[nodiscard]] constexpr void *get() noexcept
				{
					if (is_local)
						return std::bit_cast<void *>(&data);
					else
						return std::bit_cast<void *>(data);
				}
				[[nodiscard]] constexpr const void *get() const noexcept
				{
					if (is_local)
						return std::bit_cast<const void *>(&data);
					else
						return std::bit_cast<const void *>(data);
				}

				template<typename T>
				[[nodiscard]] constexpr auto *get() noexcept
				{
					return static_cast<T *>(get());
				}
				template<typename T>
				[[nodiscard]] constexpr auto *get() const noexcept
				{
					return static_cast<std::add_const_t<T> *>(get());
				}

				[[nodiscard]] storage_t ref() noexcept { return {get(), is_const}; }
				[[nodiscard]] storage_t ref() const noexcept { return {get(), is_const}; }

				constexpr void swap(storage_t &other) noexcept
				{
					std::swap(data, other.data);
					std::swap(is_ref, other.is_ref);
					std::swap(is_local, other.is_local);
					std::swap(is_const, other.is_const);
				}

				std::uintptr_t data = {};
				bool is_ref = false;
				bool is_local = false;
				bool is_const = false;
			};
			struct vtable_t
			{
				using cmp_func = bool (*)(const storage_t &, const storage_t &);
				using copy_func = void (*)(const storage_t &, storage_t &);
				using dtor_func = void (*)(storage_t &);

				template<typename T>
				constexpr static vtable_t make_instance() noexcept;
				template<typename T>
				static const vtable_t instance;

				dtor_func destroy = nullptr;
				copy_func construct = nullptr;
				copy_func assign = nullptr;

				cmp_func cmp_eq = nullptr;
				cmp_func cmp_lt = nullptr;
				cmp_func cmp_le = nullptr;
				cmp_func cmp_gt = nullptr;
				cmp_func cmp_ge = nullptr;
			};

		public:
			constexpr basic_any() noexcept = default;

			constexpr basic_any(const basic_any &) noexcept = default;
			constexpr basic_any &operator=(const basic_any &other) noexcept
			{
				if (this != &other) [[likely]]
				{
					m_vtable = other.m_vtable;
					m_type = other.m_type;
					m_storage = other.m_storage;
				}
				return *this;
			}
			constexpr basic_any(basic_any &&other) noexcept { move_init(other); }
			constexpr basic_any &operator=(basic_any &&other) noexcept
			{
				move_assign(other);
				return *this;
			}

			[[nodiscard]] basic_any ref() noexcept
			{
				basic_any result;
				result.m_vtable = m_vtable;
				result.m_type = m_type;
				result.m_storage = m_storage.ref();
				return result;
			}
			[[nodiscard]] basic_any ref() const noexcept
			{
				basic_any result;
				result.m_vtable = m_vtable;
				result.m_type = m_type;
				result.m_storage = m_storage.ref();
				return result;
			}

			void reset()
			{
				destroy();
				m_vtable = nullptr;
				m_type = nullptr;
				m_storage = {};
			}

			void destroy()
			{
				if (m_vtable != nullptr) [[likely]]
					m_vtable->destroy(m_storage);
			}
			void move_init(basic_any &other)
			{
				m_vtable = std::exchange(other.m_vtable, nullptr);
				m_type = std::exchange(other.m_type, nullptr);
				m_storage = std::move(other.m_storage);
			}
			void move_assign(basic_any &other)
			{
				std::swap(m_vtable, other.m_vtable);
				std::swap(m_type, other.m_type);
				m_storage = std::move(other.m_storage);
			}
			void copy_init(const basic_any &other) {}
			void copy_assign(const basic_any &other) {}

			constexpr void swap(basic_any &other) noexcept
			{
				std::swap(m_vtable, other.m_vtable);
				std::swap(m_type, other.m_type);
				m_storage.swap(other.m_storage);
			}

			const vtable_t *m_vtable = nullptr;
			const type_data *m_type = nullptr;
			storage_t m_storage = {};
		};

		template<typename T>
		constexpr basic_any::vtable_t basic_any::vtable_t::make_instance() noexcept
		{
			vtable_t result;

			result.destroy = +[](storage_t &s) { s.destroy<T>(); };
			if constexpr (std::copy_constructible<T>)
				result.construct = +[](const storage_t &src, storage_t &dst)
				{
					dst.template init<T>(*src.template get<T>());
					dst.template init_flags<T>();
				};
			if constexpr (std::is_copy_assignable_v<T> || std::copy_constructible<T>)
				result.assign = +[](const storage_t &src, storage_t &dst)
				{
					if constexpr (std::is_copy_assignable_v<T>)
						*dst.template get<T>() = *src.template get<T>();
					else
					{
						dst.template destroy<T>();
						dst.template init<T>(*src.template get<T>());
					}
					dst.template init_flags<T>();
				};

			if constexpr (requires(const T &a, const T &b) { a == b; })
				result.cmp_eq = +[](const storage_t &a, const storage_t &b) -> bool
				{
					const auto &a_value = *a.template get<T>();
					const auto &b_value = *b.template get<T>();
					return a_value == b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a < b; })
				result.cmp_lt = +[](const storage_t &a, const storage_t &b) -> bool
				{
					const auto &a_value = *a.template get<T>();
					const auto &b_value = *b.template get<T>();
					return a_value < b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a <= b; })
				result.cmp_le = +[](const storage_t &a, const storage_t &b) -> bool
				{
					const auto &a_value = *a.template get<T>();
					const auto &b_value = *b.template get<T>();
					return a_value <= b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a > b; })
				result.cmp_gt = +[](const storage_t &a, const storage_t &b) -> bool
				{
					const auto &a_value = *a.template get<T>();
					const auto &b_value = *b.template get<T>();
					return a_value > b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a >= b; })
				result.cmp_ge = +[](const storage_t &a, const storage_t &b) -> bool
				{
					const auto &a_value = *a.template get<T>();
					const auto &b_value = *b.template get<T>();
					return a_value >= b_value;
				};

			return result;
		}
		template<typename T>
		constinit const basic_any::vtable_t basic_any::vtable_t::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Type-erased container of objects. */
	class any : detail::basic_any
	{
		friend class any_ref;
		friend class any_tuple;
		friend class any_range;
		friend class any_table;

		friend bool SEK_API operator==(const any &a, const any &b) noexcept;
		friend bool SEK_API operator<(const any &a, const any &b) noexcept;
		friend bool SEK_API operator<=(const any &a, const any &b) noexcept;
		friend bool SEK_API operator>(const any &a, const any &b) noexcept;
		friend bool SEK_API operator>=(const any &a, const any &b) noexcept;

		friend bool SEK_API operator==(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator<(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator<=(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator>(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator>=(const any &a, const any_ref &b) noexcept;

		friend bool SEK_API operator==(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator<(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator<=(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator>(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator>=(const any_ref &a, const any &b) noexcept;

		using base_t = detail::basic_any;

		constexpr any(base_t &&data) : base_t(std::forward<base_t>(data)) {}

	public:
		/** Initializes an empty `any`. */
		constexpr any() noexcept = default;
		~any() { base_t::destroy(); }

		constexpr any(any &&other) noexcept : base_t(std::move(other)) {}
		constexpr any &operator=(any &&other) noexcept
		{
			base_t::operator=(std::move(other));
			return *this;
		}

		/** Copy-constructs the managed object of `other`.
		 * @throw type_error If the underlying type is not copy-constructable. */
		any(const any &other) { base_t::copy_init(other); }
		/** Copy-assigns the managed object of `other`.
		 * @throw type_error If the underlying type is not copy-assignable or copy-constructable. */
		any &operator=(const any &other)
		{
			if (this != &other) [[likely]]
				base_t::copy_assign(other);
			return *this;
		}

		/** Checks if the `any` instance is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_vtable == nullptr; }

		/** Returns type of the managed object, or an invalid `type_info` if empty. */
		[[nodiscard]] constexpr type_info type() const noexcept;

		/** Checks if the `any` instance contains a reference to an object. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return m_storage.is_ref; }
		/** Checks if the `any` instance manages a const-qualified object. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_storage.is_const; }

		/** Releases the managed object. */
		void reset() { base_t::reset(); }

		/** Returns raw pointer to the managed object's data.
		 * @note If the managed object is const-qualified, returns nullptr. */
		[[nodiscard]] void *data() noexcept
		{
			if (is_const()) [[unlikely]]
				return nullptr;
			return m_storage.get();
		}
		/** Returns raw const pointer to the managed object's data. */
		[[nodiscard]] const void *cdata() const noexcept { return m_storage.get(); }
		/** @copydoc cdata */
		[[nodiscard]] const void *data() const noexcept { return cdata(); }

		/** Creates an `any` instance referencing the managed object. */
		[[nodiscard]] any ref() noexcept { return base_t::ref(); }
		/** Creates an `any` instance referencing the managed object via const-reference. */
		[[nodiscard]] any ref() const noexcept { return base_t::ref(); }
		/** @copydoc ref */
		[[nodiscard]] any cref() const noexcept { return ref(); }

		/** Returns a `any_range` proxy for the managed type,
		 * `type_errc::INVALID_TYPE` if the managed type is not a range,
		 * or `type_errc::UNEXPECTED_EMPTY_ANY` if `any` is empty. */
		[[nodiscard]] expected<any_range, std::error_code> range(std::nothrow_t);
		/** @copydoc range */
		[[nodiscard]] expected<any_range, std::error_code> range(std::nothrow_t) const;
		/** Returns a `any_range` proxy for the managed type.
		 * @throw type_error If the managed type is not a range or `any` is empty. */
		[[nodiscard]] any_range range();
		/** @copydoc range */
		[[nodiscard]] any_range range() const;

		/** Returns a `any_table` proxy for the managed type,
		 * `type_errc::INVALID_TYPE` if the managed type is not a table,
		 * or `type_errc::UNEXPECTED_EMPTY_ANY` if `any` is empty. */
		[[nodiscard]] expected<any_table, std::error_code> table(std::nothrow_t);
		/** @copydoc range */
		[[nodiscard]] expected<any_table, std::error_code> table(std::nothrow_t) const;
		/** Returns a `any_table` proxy for the managed type.
		 * @throw type_error If the managed type is not a table or `any` is empty. */
		[[nodiscard]] any_table table();
		/** @copydoc table */
		[[nodiscard]] any_table table() const;

		/** Returns a `any_tuple` proxy for the managed type,
		 * `type_errc::INVALID_TYPE` if the managed type is not a tuple,
		 * or `type_errc::UNEXPECTED_EMPTY_ANY` if `any` is empty. */
		[[nodiscard]] expected<any_tuple, std::error_code> tuple(std::nothrow_t);
		/** @copydoc range */
		[[nodiscard]] expected<any_tuple, std::error_code> tuple(std::nothrow_t) const;
		/** Returns a `any_tuple` proxy for the managed type.
		 * @throw type_error If the managed type is not a tuple or `any` is empty. */
		[[nodiscard]] any_tuple tuple();
		/** @copydoc tuple */
		[[nodiscard]] any_tuple tuple() const;

		constexpr void swap(any &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(any &a, any &b) noexcept { a.swap(b); }
	};
	/** @brief Type-erased reference to objects. */
	class any_ref : detail::basic_any
	{
		friend class any_tuple;
		friend class any_range;
		friend class any_table;

		friend bool SEK_API operator==(const any_ref &a, const any_ref &b) noexcept;
		friend bool SEK_API operator<(const any_ref &a, const any_ref &b) noexcept;
		friend bool SEK_API operator<=(const any_ref &a, const any_ref &b) noexcept;
		friend bool SEK_API operator>(const any_ref &a, const any_ref &b) noexcept;
		friend bool SEK_API operator>=(const any_ref &a, const any_ref &b) noexcept;

		friend bool SEK_API operator==(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator<(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator<=(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator>(const any &a, const any_ref &b) noexcept;
		friend bool SEK_API operator>=(const any &a, const any_ref &b) noexcept;

		friend bool SEK_API operator==(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator<(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator<=(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator>(const any_ref &a, const any &b) noexcept;
		friend bool SEK_API operator>=(const any_ref &a, const any &b) noexcept;

		using base_t = detail::basic_any;

	public:
		any_ref() = delete;

		/** Initializes `any_ref` to reference an object managed by an `any` instance.
		 * @param data `any` instance to reference. */
		any_ref(any &data) noexcept { init(data.ref()); }
		/** @copydoc any_ref */
		any_ref(const any &data) noexcept { init(data.ref()); }
		/** Initializes `any_ref` from a reference `any` instance by-move.
		 * @param data `any` containing a reference to an object.
		 * @throw type_error Provided `any` must be a reference, using a non-reference `any` will result in undefined behavior. */
		any_ref(any &&data) { init(std::forward<any>(data)); }

		constexpr any_ref(const any_ref &other) noexcept : base_t(other) {}
		constexpr any_ref &operator=(const any_ref &other) noexcept
		{
			base_t::operator=(other);
			return *this;
		}
		constexpr any_ref(any_ref &&other) noexcept : base_t(std::move(other)) {}
		constexpr any_ref &operator=(any_ref &&other) noexcept
		{
			base_t::operator=(std::move(other));
			return *this;
		}
		constexpr ~any_ref() = default;

		/** Returns type of the referenced object. */
		[[nodiscard]] constexpr type_info type() const noexcept;

		/** Checks if the `any_ref` instance references a const-qualified object. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_storage.is_const; }

		/** Returns raw pointer to the referenced object's data.
		 * @note If the referenced object is const-qualified, returns nullptr. */
		[[nodiscard]] void *data() noexcept
		{
			if (is_const()) [[unlikely]]
				return nullptr;
			return std::bit_cast<void *>(m_storage.data);
		}
		/** Returns raw const pointer to the referenced object's data. */
		[[nodiscard]] const void *cdata() const noexcept { return std::bit_cast<const void *>(m_storage.data); }
		/** @copydoc cdata */
		[[nodiscard]] const void *data() const noexcept { return cdata(); }

		/** Creates an `any` instance referencing the object referenced by this `any_ref`. */
		[[nodiscard]] any ref() noexcept { return base_t::ref(); }
		/** @copydoc ref */
		[[nodiscard]] operator any() noexcept { return ref(); }
		/** Creates an `any` instance referencing the object referenced by this `any_ref` via const-reference. */
		[[nodiscard]] any ref() const noexcept { return base_t::ref(); }
		/** @copydoc ref */
		[[nodiscard]] operator any() const noexcept { return ref(); }
		/** @copydoc ref */
		[[nodiscard]] any cref() const noexcept { return ref(); }

		/** Returns a `any_range` proxy for the referenced type, or `type_errc::INVALID_TYPE` if the referenced type is not a range. */
		[[nodiscard]] expected<any_range, std::error_code> range(std::nothrow_t);
		/** @copydoc range */
		[[nodiscard]] expected<any_range, std::error_code> range(std::nothrow_t) const;
		/** Returns a `any_range` proxy for the referenced type.
		 * @throw type_error If the referenced type is not a range. */
		[[nodiscard]] any_range range();
		/** @copydoc range */
		[[nodiscard]] any_range range() const;

		/** Returns a `any_table` proxy for the referenced type, or `type_errc::INVALID_TYPE` if the referenced type is not a table. */
		[[nodiscard]] expected<any_table, std::error_code> table(std::nothrow_t);
		/** @copydoc range */
		[[nodiscard]] expected<any_table, std::error_code> table(std::nothrow_t) const;
		/** Returns a `any_table` proxy for the referenced type.
		 * @throw type_error If the referenced type is not a table. */
		[[nodiscard]] any_table table();
		/** @copydoc table */
		[[nodiscard]] any_table table() const;

		/** Returns a `any_tuple` proxy for the referenced type, or `type_errc::INVALID_TYPE` if the referenced type is not a tuple. */
		[[nodiscard]] expected<any_tuple, std::error_code> tuple(std::nothrow_t);
		/** @copydoc range */
		[[nodiscard]] expected<any_tuple, std::error_code> tuple(std::nothrow_t) const;
		/** Returns a `any_tuple` proxy for the referenced type.
		 * @throw type_error If the referenced type is not a tuple. */
		[[nodiscard]] any_tuple tuple();
		/** @copydoc tuple */
		[[nodiscard]] any_tuple tuple() const;

		constexpr void swap(any_ref &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(any_ref &a, any_ref &b) noexcept { a.swap(b); }

	private:
		void init(any &&data)
		{
			if (!data.is_ref()) [[unlikely]]
				throw type_error(make_error_code(type_errc::EXPECTED_REF_ANY));
			base_t::move_init(data);
		}
	};

	/** If the managed objects of `a` and `b` are of the same type that is equality comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator==(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is less-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is less-than-or-equal comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<=(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is greater-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is greater-than-or-equal comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>=(const any &a, const any &b) noexcept;

	/** If the referenced objects of `a` and `b` are of the same type that is equality comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator==(const any_ref &a, const any_ref &b) noexcept;
	/** If the referenced objects of `a` and `b` are of the same type that is less-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<(const any_ref &a, const any_ref &b) noexcept;
	/** If the referenced objects of `a` and `b` are of the same type that is less-than-or-equal comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<=(const any_ref &a, const any_ref &b) noexcept;
	/** If the referenced objects of `a` and `b` are of the same type that is greater-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>(const any_ref &a, const any_ref &b) noexcept;
	/** If the referenced objects of `a` and `b` are of the same type that is greater-than-or-equal comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>=(const any_ref &a, const any_ref &b) noexcept;

	/** If object referenced by `a` and object managed by `b` are of the same type that is equality comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator==(const any_ref &a, const any &b) noexcept;
	/** If object referenced by `a` and object managed by `b` are of the same type that is less-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<(const any_ref &a, const any &b) noexcept;
	/** If object referenced by `a` and object managed by `b` are of the same type that is less-than-or-equal
	 * comparable, returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<=(const any_ref &a, const any &b) noexcept;
	/** If object referenced by `a` and object managed by `b` are of the same type that is greater-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>(const any_ref &a, const any &b) noexcept;
	/** If object referenced by `a` and object managed by `b` are of the same type that is greater-than-or-equal
	 * comparable, returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>=(const any_ref &a, const any &b) noexcept;

	/** If object managed by `a` and object referenced by `b` are of the same type that is equality comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator==(const any &a, const any_ref &b) noexcept;
	/** If object managed by `a` and object referenced by `b` are of the same type that is less-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<(const any &a, const any_ref &b) noexcept;
	/** If object managed by `a` and object referenced by `b` are of the same type that is less-than-or-equal
	 * comparable, returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator<=(const any &a, const any_ref &b) noexcept;
	/** If object managed by `a` and object referenced by `b` are of the same type that is greater-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>(const any &a, const any_ref &b) noexcept;
	/** If object managed by `a` and object referenced by `b` are of the same type that is greater-than-or-equal
	 * comparable, returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_API operator>=(const any &a, const any_ref &b) noexcept;
}	 // namespace sek
