/*
 * Created by switchblade on 17/06/22
 */

#pragma once

#include <concepts>
#include <mutex>
#include <optional>
#include <utility>

#include <shared_mutex>

namespace sek
{
	namespace detail
	{
		// clang-format off
		template<typename T>
		concept basic_lockable = requires(T &value)
		{
			value.lock();
			value.unlock();
		};
		template<typename T>
		concept lockable = requires(T &value)
		{
			requires basic_lockable<T>;
			{ value.try_lock() } -> std::same_as<bool>;
		};
		template<typename T>
		concept shared_lockable = requires(T &value)
		{
			requires lockable<T>;
			value.lock_shared();
			value.unlock_shared();
			{ value.try_lock_shared() } -> std::same_as<bool>;
		};
		// clang-format on
	}	 // namespace detail

	template<typename T, detail::basic_lockable Mutex, bool>
	class access_guard;

	/** @brief Pointer-like accessor returned by `access_guard`. */
	template<typename T, typename L>
	class access_handle
	{
		template<typename, detail::basic_lockable, bool>
		friend class access_guard;

	public:
		typedef T element_type;
		typedef std::remove_reference_t<T> *pointer;
		typedef std::remove_reference_t<T> &reference;

	public:
		access_handle() = delete;
		access_handle(const access_handle &) = delete;
		access_handle &operator=(const access_handle &) = delete;

		constexpr access_handle(access_handle &&other) noexcept(std::is_nothrow_move_constructible_v<L>)
			: m_ptr(std::exchange(other.m_ptr, nullptr)), m_lock(std::move(other.m_lock))
		{
		}
		constexpr access_handle &operator=(access_handle &&other) noexcept(std::is_nothrow_move_assignable_v<L>)
		{
			m_ptr = std::exchange(other.m_ptr, nullptr);
			m_lock = std::move(other.m_lock);
			return *this;
		}

		/** Initializes an access handle for reference `ref` and lock `lock`. */
		constexpr access_handle(reference ref, L &&lock) noexcept(std::is_nothrow_move_constructible_v<L>)
			: m_ptr(std::addressof(ref)), m_lock(std::move(lock))
		{
		}

		/** Returns pointer to the underlying object. */
		[[nodiscard]] constexpr pointer get() const noexcept { return m_ptr; }
		/** @copydoc get */
		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		/** Returns reference to the underlying object. */
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

	private:
		pointer m_ptr;
		L m_lock;
	};

	/** @brief Smart pointer used to provide synchronized access to an instance of a type.
	 * @tparam T Value type stored pointed to by the access guard.
	 * @tparam Mutex Type of mutex used to synchronize instance of `T`. */
	template<typename T, detail::basic_lockable Mutex = std::mutex, bool = detail::shared_lockable<Mutex>>
	class access_guard
	{
	public:
		typedef T value_type;
		typedef Mutex mutex_type;

		typedef std::unique_lock<Mutex> unique_lock;
		typedef access_handle<T, unique_lock> unique_handle;

	public:
		/** Initializes an empty access guard. */
		constexpr access_guard() noexcept = default;

		/** Initializes an access guard for an object and mutex instance. */
		constexpr access_guard(value_type *value, mutex_type *mtx) noexcept : m_value(value), m_mtx(mtx) {}
		/** @copydoc access_guard */
		constexpr access_guard(value_type &value, mutex_type &mtx) noexcept
			: access_guard(std::addressof(value), std::addressof(mtx))
		{
		}

		/** Checks if the access guard is empty (does not point to any object). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_value == nullptr || m_mtx == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Acquires a unique lock and returns an accessor handle. */
		[[nodiscard]] constexpr unique_handle access() { return unique_handle{*m_value, unique_lock{*m_mtx}}; }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle operator->() { return access(); }

		// clang-format off
		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<unique_handle> try_access() requires detail::lockable<Mutex>
		{
			if (m_mtx->try_lock()) [[likely]]
				return {unique_handle{*m_value, unique_lock{*m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
		// clang-format on

		/** Returns pointer to the underlying value. */
		[[nodiscard]] constexpr value_type *value() noexcept { return m_value; }
		/** @copydoc value */
		[[nodiscard]] constexpr const value_type *value() const noexcept { return m_value; }

		/** Returns pointer to the underlying mutex. */
		[[nodiscard]] constexpr mutex_type *mutex() noexcept { return m_mtx; }
		/** @copydoc mutex */
		[[nodiscard]] constexpr const mutex_type *mutex() const noexcept { return m_mtx; }

	protected:
		value_type *m_value = {};
		mutex_type *m_mtx = {};
	};
	/** @brief Overload of `access_guard` for shared mutex types. */
	template<typename T, detail::basic_lockable Mutex>
	class access_guard<T, Mutex, true> : public access_guard<T, Mutex, false>
	{
		using base_t = access_guard<T, Mutex, false>;

	public:
		typedef std::shared_lock<Mutex> shared_lock;
		typedef access_handle<std::add_const_t<T>, shared_lock> shared_handle;

	public:
		using access_guard<T, Mutex, false>::operator=;
		using access_guard<T, Mutex, false>::operator->;
		using access_guard<T, Mutex, false>::access_guard;

		/** Acquires a shared lock and returns an accessor handle. */
		[[nodiscard]] constexpr shared_handle access_shared() const
		{
			return shared_handle{*base_t::m_value, shared_lock{*base_t::m_mtx}};
		}
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle operator->() const { return access_shared(); }

		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<shared_handle> try_access_shared() const
		{
			if (base_t::m_mtx.try_lock_shared()) [[likely]]
				return {shared_handle{*base_t::m_value, shared_lock{*base_t::m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
	};

	template<typename T, typename M>
	access_guard(const T &, const M &) -> access_guard<T, M>;

	/** @brief Type alias used to define an `access_guard` that uses an `std::shared_mutex` as its mutex type. */
	template<typename T>
	using shared_guard = access_guard<T, std::shared_mutex>;
}	 // namespace sek