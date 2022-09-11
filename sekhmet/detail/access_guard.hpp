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

	/** @brief Helper structure used to wrap an external mutex for use with `access_guard`. */
	template<detail::basic_lockable Mutex>
	class mutex_ref
	{
	public:
		typedef Mutex mutex_type;

	public:
		mutex_ref() = delete;

		constexpr mutex_ref(const mutex_ref &) noexcept = default;
		constexpr mutex_ref &operator=(const mutex_ref &) noexcept = default;
		constexpr mutex_ref(mutex_ref &&) noexcept = default;
		constexpr mutex_ref &operator=(mutex_ref &&) noexcept = default;

		constexpr mutex_ref(mutex_type &mtx) noexcept : m_ptr(std::addressof(mtx)) {}

		/** Invokes `lock` on the underlying mutex. */
		constexpr void lock() { m_ptr->lock(); }
		/** Invokes `unlock` on the underlying mutex. */
		constexpr void unlock() { m_ptr->unlock(); }

		// clang-format off
		/** Invokes `try_lock` on the underlying mutex. */
		constexpr bool try_lock() requires detail::lockable<Mutex> { return m_ptr->try_lock(); }

		/** Invokes `lock_shared` on the underlying mutex. */
		constexpr void lock_shared() requires detail::shared_lockable<Mutex> { m_ptr->lock_shared(); }
		/** Invokes `unlock_shared` on the underlying mutex. */
		constexpr void unlock_shared() requires detail::shared_lockable<Mutex> { m_ptr->unlock_shared(); }
		/** Invokes `try_lock_shared` on the underlying mutex. */
		constexpr bool try_lock_shared() requires detail::shared_lockable<Mutex> { return m_ptr->try_lock_shared(); }
		// clang-format on

		/** Returns reference to the underlying mutex. */
		[[nodiscard]] constexpr mutex_type &get() noexcept { return *m_ptr; }
		/** @copydoc get */
		[[nodiscard]] constexpr const mutex_type &get() const noexcept { return *m_ptr; }

	private:
		mutex_type *m_ptr;
	};

	template<typename M>
	mutex_ref(M &) -> mutex_ref<M>;

	template<typename T, detail::basic_lockable Mutex, bool>
	class access_guard;

	/** @brief Pointer-like accessor returned by `access_guard`. */
	template<typename T, typename L>
	class access_handle
	{
		template<typename, detail::basic_lockable>
		friend class access_guard;
		template<typename>
		friend class std::optional;

	public:
		typedef T element_type;
		typedef std::remove_reference_t<T> *pointer;
		typedef std::remove_reference_t<T> &reference;

	private:
		constexpr access_handle(reference ref, L &&lock) noexcept(std::is_nothrow_move_constructible_v<L>)
			: m_ptr(std::addressof(ref)), m_lock(std::move(lock))
		{
		}

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
		pointer m_ptr;
		L m_lock;
	};

	/** @brief Structure used to provide synchronized access to an instance of a type.
	 * @tparam T Value type stored within the access guard.
	 * @tparam Mutex Type of mutex used to synchronize instance of `T`. */
	template<typename T, detail::basic_lockable Mutex = std::mutex, bool = false>
	class access_guard
	{
	public:
		typedef T value_type;
		typedef Mutex mutex_type;

		typedef std::unique_lock<Mutex> unique_lock;
		typedef access_handle<T, unique_lock> unique_handle;

	private:
		template<std::size_t... Is, typename U, typename... Args>
		[[nodiscard]] constexpr static U make_value(std::index_sequence<Is...>, std::tuple<Args...> args)
		{
			return T{std::forward<std::tuple_element_t<Is, std::tuple<Args...>>>(std::get<Is>(args))...};
		}
		template<typename U, typename... Args>
		[[nodiscard]] constexpr static U make_value(std::tuple<Args...> args)
		{
			return make_value<U>(std::make_index_sequence<sizeof...(Args)>{}, args);
		}

	public:
		access_guard(const access_guard &) = delete;
		access_guard &operator=(const access_guard &) = delete;

		constexpr access_guard(access_guard &&other) noexcept = default;
		constexpr access_guard &operator=(access_guard &&other) noexcept = default;

		// clang-format off
		constexpr access_guard()
			noexcept(noexcept(mutex_type{}) && noexcept(value_type{}))
			requires(std::is_default_constructible_v<mutex_type> &&
			         std::is_default_constructible_v<value_type>) = default;

		/** Initializes value in-place.
		 * @param args Arguments passed to constructor of the value type. */
		template<typename... Args>
		constexpr explicit access_guard(std::in_place_t, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<value_type, Args...> &&
			         std::is_nothrow_default_constructible_v<mutex_type>)
			requires std::constructible_from<value_type, Args...>
			: m_value(std::forward<Args>(args)...)
		{
		}
		/** Initializes both value an mutex in-place.
		 * @param val_args Arguments passed to constructor of the value type.
		 * @param mtx_args Arguments passed to constructor of the mutex type. */
		template<typename... VArgs, typename... MArgs>
		constexpr access_guard(std::piecewise_construct_t, std::tuple<VArgs...> val_args, std::tuple<MArgs...> mtx_args)
			noexcept(std::is_nothrow_constructible_v<value_type, VArgs...> &&
					 std::is_nothrow_constructible_v<mutex_type, MArgs...>)
			requires(std::is_constructible_v<value_type, VArgs...> &&
					 std::is_constructible_v<mutex_type, MArgs...>)
			: m_value(make_value<value_type>(val_args)), m_mtx(make_value<mutex_type>(mtx_args))
		{
		}

		/** Initializes the value from passed argument. */
		template<typename U = value_type>
		constexpr access_guard(U &&value)
			noexcept(std::is_nothrow_default_constructible_v<mutex_type> &&
			         std::is_nothrow_constructible_v<value_type, U>)
			requires(std::is_default_constructible_v<mutex_type> &&
                     std::is_constructible_v<value_type, U>)
			: m_value(std::forward<U>(value))
		{
		}
		/** Initializes the value and the mutex from passed arguments. */
		template<typename U = value_type, typename V = mutex_type>
		constexpr access_guard(U &&value, V &&mtx)
			noexcept(std::is_nothrow_constructible_v<value_type, U> &&
					 std::is_nothrow_constructible_v<mutex_type, V>)
			requires(std::is_constructible_v<value_type, U> &&
					 std::is_constructible_v<mutex_type, V>)
			: m_value(std::forward<U>(value)), m_mtx(std::forward<V>(mtx))
		{
		}
		// clang-format on

		/** Acquires a unique lock and returns an accessor handle. */
		[[nodiscard]] constexpr unique_handle access() { return unique_handle{m_value, unique_lock{m_mtx}}; }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle operator->() { return access(); }

		// clang-format off
		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<unique_handle> try_access() requires detail::lockable<Mutex>
		{
			if (m_mtx.try_lock()) [[likely]]
				return {unique_handle{m_value, unique_lock{m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
		// clang-format on

		/** Returns reference to the underlying value. */
		[[nodiscard]] constexpr std::remove_reference_t<value_type> &value() noexcept { return m_value; }
		/** @copydoc value */
		[[nodiscard]] constexpr const std::remove_cvref_t<value_type> &value() const noexcept { return m_value; }

		/** Returns reference to the underlying mutex. */
		[[nodiscard]] constexpr mutex_type &mutex() noexcept { return m_mtx; }
		/** @copydoc mutex */
		[[nodiscard]] constexpr const mutex_type &mutex() const noexcept { return m_mtx; }

	private:
		value_type m_value = {};
		mutable mutex_type m_mtx = {};
	};

	/** @brief Overload of `access_guard` for shared mutex types. */
	template<typename T, detail::basic_lockable Mutex>
	class access_guard<T, Mutex, detail::shared_lockable<Mutex>> : public access_guard<T, Mutex, false>
	{
		using base_t = access_guard<T, Mutex, false>;

	public:
		typedef std::shared_lock<Mutex> shared_lock;
		typedef access_handle<std::add_const_t<T>, shared_lock> shared_handle;

	public:
		using base_t::operator=;
		using base_t::access_guard;

		/** Acquires a shared lock and returns an accessor handle. */
		[[nodiscard]] constexpr shared_handle access_shared() const
		{
			return shared_handle{base_t::m_value, shared_lock{base_t::m_mtx}};
		}
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle operator->() const { return access_shared(); }

		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<shared_handle> try_access_shared() const
		{
			if (base_t::m_mtx.try_lock_shared()) [[likely]]
				return {shared_handle{base_t::m_value, shared_lock{base_t::m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
	};

	template<typename T, typename M>
	access_guard(const T &, const M &) -> access_guard<T, M>;

	/** @brief Type alias used to define an `access_guard` that uses an `std::shared_mutex`.as its mutex type. */
	template<typename T>
	using shared_guard = access_guard<T, std::shared_mutex>;
	/** @brief Type alias used to define an `access_guard` that uses a `mutex_ref`.reference wrapper to `M` as its mutex type. */
	template<typename T, typename M>
	using ref_guard = access_guard<T, mutex_ref<M>>;
}	 // namespace sek