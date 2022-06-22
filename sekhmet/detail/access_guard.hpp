/*
 * Created by switchblade on 17/06/22
 */

#pragma once

#include <concepts>
#include <mutex>
#include <optional>

#include <shared_mutex>

namespace sek
{
	namespace detail
	{
		// clang-format off
		template<typename T>
		concept basic_lockable = requires(T &value) {
									 value.lock();
									 value.unlock();
								 };
		template<typename T>
		concept lockable = requires(T &value) {
							   requires basic_lockable<T>;
							   { value.try_lock() } -> std::same_as<bool>;
						   };
		template<typename T>
		concept shared_lockable = requires(T &value) {
									  requires lockable<T>;
									  value.lock_shared();
									  value.unlock_shared();
									  { value.try_lock_shared() } -> std::same_as<bool>;
								  };
		// clang-format on
	}	 // namespace detail

	template<typename T, detail::basic_lockable Mutex>
	class access_guard;

	/** @brief Pointer-like accessor returned by `access_guard`. */
	template<typename T, typename L>
	class access_handle
	{
		template<typename, detail::basic_lockable>
		friend class access_guard;
		template<typename>
		friend class std::optional;

		constexpr access_handle(T &ref, L &&lock) noexcept(std::is_nothrow_move_constructible_v<L>)
			: m_ptr(std::addressof(ref)), m_lock(std::move(lock))
		{
		}

	public:
		typedef T element_type;
		typedef T *pointer;
		typedef T &reference;

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

		/** Returns pointer to the underlying object. */
		[[nodiscard]] constexpr pointer get() const noexcept { return m_ptr; }
		/** @copydoc get */
		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		/** Returns reference to the underlying object. */
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

	private:
		T *m_ptr;
		L m_lock;
	};

	namespace detail
	{
		template<typename T, typename M>
		struct access_guard_typedef
		{
			typedef std::unique_lock<M> unique_lock;
			typedef access_handle<T, unique_lock> unique_handle;
		};
		template<typename T, shared_lockable M>
		struct access_guard_typedef<T, M>
		{
			typedef std::unique_lock<M> unique_lock;
			typedef access_handle<T, unique_lock> unique_handle;
			typedef std::shared_lock<M> shared_lock;
			typedef access_handle<const T, shared_lock> shared_handle;
		};
	}	 // namespace detail

	/** @brief Structure used to provide synchronized access to an instance of a type.
	 * @tparam T Value type stored within the access guard.
	 * @tparam Mutex Type of mutex used to synchronize instance of `T`. */
	template<typename T, detail::basic_lockable Mutex = std::mutex>
	class access_guard : public detail::access_guard_typedef<T, Mutex>
	{
	public:
		typedef T value_type;
		typedef Mutex mutex_type;

	private:
		constexpr static bool allow_shared = detail::shared_lockable<Mutex>;
		constexpr static bool allow_try = detail::lockable<Mutex>;

		using typedef_base = detail::access_guard_typedef<T, Mutex>;

	public:
		access_guard(const access_guard &) = delete;
		access_guard(access_guard &&) = delete;
		access_guard &operator=(const access_guard &) = delete;
		access_guard &operator=(access_guard &&) = delete;

		// clang-format off
		constexpr access_guard() noexcept(noexcept(mutex_type{}) && noexcept(value_type{})) = default;

		/** Initializes value type in-place.
		 * @param args Arguments passed to constructor of the value type. */
		template<typename... Args>
		constexpr explicit access_guard(std::in_place_t, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<value_type, Args...> &&
			         std::is_nothrow_default_constructible_v<mutex_type>)
			requires std::constructible_from<value_type, Args...>
			: m_value(std::forward<Args>(args)...)
		{
		}
		/** Initializes value by-copy. */
		constexpr access_guard(const value_type &value)
			noexcept(std::is_nothrow_copy_constructible_v<value_type> &&
			         std::is_nothrow_default_constructible_v<mutex_type>)
			: m_value(value)
		{
		}
		/** Initializes value by-move. */
		constexpr access_guard(value_type &&value)
			noexcept(std::is_nothrow_move_constructible_v<value_type> &&
			         std::is_nothrow_default_constructible_v<mutex_type>)
			: m_value(std::move(value))
		{
		}

		/** Acquires a unique lock and returns an accessor handle. */
		[[nodiscard]] constexpr auto access_unique()
		{
			using handle_t = typename typedef_base::unique_handle;
			using lock_t = typename typedef_base::unique_lock;
			return handle_t{m_value, lock_t{m_mtx}};
		}
		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr auto try_access_unique() requires allow_try
		{
			using handle_t = typename typedef_base::unique_handle;
			using lock_t = typename typedef_base::unique_lock;
			using result_t = std::optional<handle_t>;
			if (m_mtx.try_lock()) [[likely]]
				return result_t{handle_t{m_value, lock_t{m_mtx, std::adopt_lock}}};
			else
				return result_t{std::nullopt};
		}

		/** Acquires a shared lock and returns an accessor handle. */
		[[nodiscard]] constexpr auto access_shared() requires allow_shared
		{
			using handle_t = typename typedef_base::shared_handle;
			using lock_t = typename typedef_base::shared_lock;
			return handle_t{m_value, lock_t{m_mtx}};
		}
		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr auto try_access_shared() requires allow_shared
		{
			using handle_t = typename typedef_base::shared_handle;
			using lock_t = typename typedef_base::shared_lock;
			using result_t = std::optional<handle_t>;
			if (m_mtx.try_lock_shared()) [[likely]]
				return result_t{handle_t{m_value, lock_t{m_mtx, std::adopt_lock}}};
			else
				return result_t{std::nullopt};
		}
		// clang-format on

	private:
		value_type m_value = {};
		mutex_type m_mtx = {};
	};
}	 // namespace sek