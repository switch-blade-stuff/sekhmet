/*
 * Created by switchblade on 2022-01-23
 */

#pragma once

#include <new>
#include <span>
#include <stdexcept>
#include <utility>

#include "sekhmet/access_guard.hpp"
#include "sekhmet/detail/aligned_storage.hpp"
#include "sekhmet/detail/assert.hpp"
#include "sekhmet/dense_map.hpp"
#include "sekhmet/dense_set.hpp"
#include "sekhmet/detail/meta_util.hpp"
#include "sekhmet/service.hpp"

#include "type_name.hpp"
#include <shared_mutex>

namespace sek::engine
{
	class any;
	class any_ref;
	class type_info;
	class type_query;
	class type_database;

	template<typename T>
	[[nodiscard]] any forward_any(T &&);
	template<typename T, typename... Args>
	[[nodiscard]] any make_any(Args &&...);
	template<typename T, typename U, typename... Args>
	[[nodiscard]] any make_any(std::initializer_list<U>, Args &&...);

	namespace detail
	{
		struct type_data;
		struct type_handle
		{
			constexpr type_handle() noexcept = default;
			template<typename T>
			constexpr explicit type_handle(type_selector_t<T>) noexcept;

			[[nodiscard]] constexpr type_data *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr type_data &operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr bool operator==(const type_handle &other) const noexcept;

			type_data *(*get)() noexcept = nullptr;
		};

		struct type_data
		{
			enum flags_t : std::int32_t
			{
				NO_FLAGS = 0,
				IS_EMPTY = 0x1,
				HAS_EXTENT = 0x2,
				IS_ARRAY = 0x4,
				IS_RANGE = 0x8,
				IS_POINTER = 0x10,
				IS_POINTER_LIKE = 0x20,
			};

			template<typename T>
			struct basic_node
			{
				const T *next = nullptr;
			};
			struct ctor_node : basic_node<ctor_node>
			{
				template<typename T>
				struct default_instance
				{
					constinit static const ctor_node value;
				};

				constexpr explicit ctor_node(any (*invoke)(std::span<any>)) noexcept : invoke(invoke) {}

				std::span<type_handle> arg_types;
				any (*invoke)(std::span<any>);
			};
			template<typename T, typename... Args>
			struct ctor_instance : ctor_node
			{
				constinit static ctor_instance value;

				constexpr ctor_instance(std::array<type_handle, sizeof...(Args)> arg_ts, any (*invoke)(std::span<any>)) noexcept
					: ctor_node(invoke), arg_ts_array(arg_ts)
				{
					ctor_node::arg_types = {arg_ts_array};
				}

				std::array<type_handle, sizeof...(Args)> arg_ts_array;
			};
			struct func_node : basic_node<func_node>
			{
				constexpr func_node(type_handle ret_type, any (*invoke)(any, std::span<any>)) noexcept
					: ret_type(ret_type), invoke(invoke)
				{
				}

				std::string_view name;
				type_handle ret_type;
				std::span<type_handle> arg_types;
				any (*invoke)(any, std::span<any>);
			};
			template<typename T, auto F, typename R, typename... Args>
			struct func_instance : func_node
			{
				constexpr func_instance(type_handle ret_type,
										std::array<type_handle, sizeof...(Args)> arg_ts,
										any (*invoke)(any, std::span<any>)) noexcept
					: func_node(ret_type, invoke), arg_ts_array(arg_ts)
				{
					func_node::arg_types = {arg_ts_array};
				}

				std::array<type_handle, sizeof...(Args)> arg_ts_array;
			};
			struct attrib_node : basic_node<attrib_node>
			{
				constexpr explicit attrib_node(type_handle type) : type(type) {}
				~attrib_node()
				{
					if (destroy != nullptr) [[likely]]
						destroy(this);
				}

				type_handle type;
				any_ref (*get_any)(const attrib_node *) = nullptr;
				void (*destroy)(attrib_node *) = nullptr;
			};
			template<typename T, typename A, auto...>
			struct attrib_instance;
			struct parent_node : basic_node<parent_node>
			{
				constexpr parent_node(type_handle type, any_ref (*cast)(any_ref)) noexcept : type(type), cast(cast) {}

				type_handle type;
				any_ref (*cast)(any_ref);
			};
			struct conv_node : basic_node<conv_node>
			{
				constexpr conv_node(type_handle type, any (*convert)(any)) noexcept : type(type), convert(convert) {}

				type_handle type;
				any (*convert)(any);
			};

			template<typename T>
			struct node_list
			{
				struct iterator
				{
					typedef T value_type;
					typedef const T *pointer;
					typedef const T *const_pointer;
					typedef const T &reference;
					typedef const T &const_reference;
					typedef std::ptrdiff_t difference_type;
					typedef std::size_t size_type;
					typedef std::forward_iterator_tag iterator_category;

					constexpr iterator() noexcept = default;
					constexpr explicit iterator(pointer node) noexcept : node(node) {}

					constexpr iterator &operator++() noexcept
					{
						node = node->next;
						return *this;
					}
					constexpr iterator operator++(int) noexcept
					{
						auto temp = *this;
						++(*this);
						return temp;
					}

					[[nodiscard]] constexpr pointer operator->() const noexcept { return node; }
					[[nodiscard]] constexpr reference operator*() const noexcept { return *node; }

					[[nodiscard]] constexpr bool operator==(const iterator &) const noexcept = default;

					pointer node = nullptr;
				};

				typedef typename iterator::value_type value_type;
				typedef typename iterator::pointer pointer;
				typedef typename iterator::const_pointer const_pointer;
				typedef typename iterator::reference reference;
				typedef typename iterator::const_reference const_reference;
				typedef typename iterator::difference_type difference_type;
				typedef typename iterator::size_type size_type;

				[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{front}; }
				[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
				[[nodiscard]] constexpr iterator end() const noexcept { return iterator{}; }
				[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

				constexpr void insert(T &node) noexcept
				{
					node.next = front;
					front = &node;
				}

				const T *front = nullptr;
			};

			[[nodiscard]] constexpr bool is_empty() const noexcept { return flags & IS_EMPTY; }
			[[nodiscard]] constexpr bool has_extent() const noexcept { return flags & HAS_EXTENT; }
			[[nodiscard]] constexpr bool is_array() const noexcept { return flags & IS_ARRAY; }
			[[nodiscard]] constexpr bool is_range() const noexcept { return flags & IS_RANGE; }
			[[nodiscard]] constexpr bool is_pointer() const noexcept { return flags & IS_POINTER; }
			[[nodiscard]] constexpr bool is_pointer_like() const noexcept { return flags & IS_POINTER_LIKE; }

			template<typename T, typename... Args>
			void add_ctor() noexcept;
			template<typename T, auto F, typename R, typename... Args>
			void add_func(type_seq_t<Args...>, std::string_view) noexcept;
			template<typename T, typename P>
			void add_parent() noexcept;
			template<typename T, typename U>
			void add_conv() noexcept;
			template<typename T, auto Value, typename A, typename... As>
			void add_attrib(type_seq_t<A, As...>);
			template<typename T, typename A, typename... As>
			void add_attrib(type_seq_t<A, As...>, auto &&);

			const std::string_view name;
			const std::size_t extent;
			const type_handle value_type; /* Underlying value type of either a pointer or a range. */
			const flags_t flags;

			node_list<ctor_node> constructors = {};
			node_list<func_node> funcs = {};

			node_list<parent_node> parents = {};
			node_list<conv_node> convs = {};
			node_list<attrib_node> attribs = {};
		};

		template<typename T>
		constexpr type_data make_type_data() noexcept;

		template<typename V>
		class type_node_iterator
		{
			friend class type_info;

		public:
			typedef V value_type;
			typedef const V *pointer;
			typedef const V *const_pointer;
			typedef V reference;
			typedef V const_reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::size_t size_type;
			typedef std::forward_iterator_tag iterator_category;

		public:
			constexpr explicit type_node_iterator(V node_value) noexcept : m_value(node_value) {}
			constexpr type_node_iterator() noexcept = default;

			constexpr type_node_iterator &operator++() noexcept
			{
				m_value.m_node = node()->next;
				return *this;
			}
			constexpr type_node_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}

			[[nodiscard]] constexpr pointer operator->() const noexcept { return &m_value; }
			[[nodiscard]] constexpr reference operator*() const noexcept { return m_value; }

			[[nodiscard]] constexpr bool operator==(const type_node_iterator &other) const noexcept
			{
				return m_value == other.m_value;
			}

		private:
			[[nodiscard]] constexpr auto *node() const noexcept { return m_value.m_node; }

			value_type m_value;
		};

		/* Custom view, as CLang has issues with `std::ranges::subrange` at this time. */
		template<typename Iter>
		class type_data_view
		{
		public:
			typedef typename Iter::value_type value_type;
			typedef typename Iter::pointer pointer;
			typedef typename Iter::const_pointer const_pointer;
			typedef typename Iter::reference reference;
			typedef typename Iter::const_reference const_reference;
			typedef typename Iter::difference_type difference_type;
			typedef typename Iter::size_type size_type;

			typedef Iter iterator;
			typedef Iter const_iterator;

		public:
			constexpr type_data_view() noexcept = default;
			constexpr type_data_view(iterator first, iterator last) noexcept : m_first(first), m_last(last) {}

			[[nodiscard]] constexpr iterator begin() const noexcept { return m_first; }
			[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr iterator end() const noexcept { return m_last; }
			[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

			[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
			[[nodiscard]] constexpr reference back() const noexcept
				requires std::bidirectional_iterator<Iter>
			{
				return *std::prev(end());
			}
			[[nodiscard]] constexpr reference at(size_type i) const noexcept
				requires std::random_access_iterator<Iter>
			{
				return begin()[static_cast<difference_type>(i)];
			}
			[[nodiscard]] constexpr reference operator[](size_type i) const noexcept
				requires std::random_access_iterator<Iter>
			{
				return at(i);
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
			[[nodiscard]] constexpr size_type size() const noexcept
			{
				return static_cast<size_type>(std::distance(begin(), end()));
			}

			[[nodiscard]] constexpr bool operator==(const type_data_view &other) const noexcept
			{
				return std::equal(m_first, m_last, other.m_first, other.m_last);
			}

		private:
			Iter m_first;
			Iter m_last;
		};

		// clang-format off
		template<typename... Ts>
		concept any_args = std::conjunction_v<std::disjunction<std::is_same<std::decay_t<Ts>, any>, std::is_same<std::decay_t<Ts>, any_ref>>...>;
		// clang-format on
	}	 // namespace detail

	/** @brief Base type for all reflection-related exceptions. */
	class SEK_API type_info_error : public std::runtime_error
	{
	public:
		type_info_error() : std::runtime_error("Unknown reflection error") {}
		explicit type_info_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit type_info_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit type_info_error(const char *msg) : std::runtime_error(msg) {}
		~type_info_error() override;
	};
	/** @brief Exception thrown when a reflected type does not have the specified member function/constructor. */
	class SEK_API invalid_member : public type_info_error
	{
	public:
		invalid_member() : type_info_error("Unknown type member") {}
		explicit invalid_member(std::string &&msg) : type_info_error(std::move(msg)) {}
		explicit invalid_member(const std::string &msg) : type_info_error(msg) {}
		explicit invalid_member(const char *msg) : type_info_error(msg) {}
		~invalid_member() override;
	};
	/** @brief Exception thrown when the const-ness of `any` is invalid (expected non-const but got const object). */
	class SEK_API bad_any_const : public type_info_error
	{
	public:
		bad_any_const() : type_info_error("Invalid const-ness of `any` object") {}
		explicit bad_any_const(std::string &&msg) : type_info_error(std::move(msg)) {}
		explicit bad_any_const(const std::string &msg) : type_info_error(msg) {}
		explicit bad_any_const(const char *msg) : type_info_error(msg) {}
		~bad_any_const() override;
	};
	/** @brief Exception thrown when the type of `any` is not as expected. */
	class SEK_API bad_any_type : public type_info_error
	{
	public:
		bad_any_type() : type_info_error("Invalid type of `any` object") {}
		explicit bad_any_type(std::string &&msg) : type_info_error(std::move(msg)) {}
		explicit bad_any_type(const std::string &msg) : type_info_error(msg) {}
		explicit bad_any_type(const char *msg) : type_info_error(msg) {}
		~bad_any_type() override;
	};

	/** @brief Structure used to represent a signature of a constructor or a function. */
	class signature_info;
	/** @brief Structure used to represent information about a constructor of a reflected type. */
	class constructor_info;
	/** @brief Structure used to represent information about a function of a reflected type. */
	class function_info;
	/** @brief Structure used to represent information about an attribute of a reflected type. */
	class attribute_info;
	/** @brief Structure used to represent information about a parent-child relationship between reflected types. */
	class parent_info;
	/** @brief Structure used to represent information about a conversion cast of a reflected type. */
	class conversion_info;

	/** @brief Structure used to create reflected type information. */
	template<typename T, typename... Attr>
	class type_factory
	{
		template<typename, typename...>
		friend class type_factory;
		friend class type_database;

		using handle_t = detail::type_handle;

		constexpr explicit type_factory(handle_t handle) noexcept : m_handle(handle) {}

		// clang-format off
		template<typename U>
		constexpr static bool good_cast = requires(T v) { static_cast<U>(v); };
		template<typename U>
		constexpr static bool unqualified = std::same_as<std::remove_cvref_t<U>, U>;
		template<typename U>
		constexpr static bool different = !std::same_as<std::remove_cvref_t<U>, T>;
		template<auto F>
		constexpr static bool member_func = requires {
												typename func_traits<F>::instance_type;
												requires std::same_as<std::remove_cv_t<typename func_traits<F>::instance_type>, T>;
											};
		template<auto F>
		constexpr static bool free_func = std::is_function_v<decltype(F)>;
		// clang-format on

	public:
		constexpr ~type_factory() { submit(); }
		constexpr type_factory(const type_factory &) noexcept = default;
		constexpr type_factory &operator=(const type_factory &) noexcept = default;
		constexpr type_factory(type_factory &&) noexcept = default;
		constexpr type_factory &operator=(type_factory &&) noexcept = default;

		// clang-format off
		/** Adds a constructor to `T`'s list of constructors.
		 * @tparam Args Signature of the constructor. */
		template<typename... Args>
		type_factory &constructor() requires std::constructible_from<T, Args...>
		{
			m_handle->template add_ctor<T, Args...>();
			return *this;
		}
		/** Adds a member or member function to `T`'s list of function.
		 * @note Free functions accepting a pointer to `T` as their first argument will be treated like member functions.
		 * @param name Name of the function. This name will be used to invoke the function at runtime. */
		template<auto F>
		type_factory &function(std::string_view name) requires(member_func<F> || free_func<F>)
		{
			using traits = func_traits<F>;
			m_handle->template add_func<T, F, typename traits::return_type>(typename traits::arg_types{}, name);
			return *this;
		}
		/** Adds `P` to the list of parents of `T`.
		 * @tparam P Parent type of `T`. */
		template<typename P>
		type_factory &parent() requires std::derived_from<T, P> && unqualified<P> && different<P>
		{
			m_handle->template add_parent<T, P>();
			return *this;
		}
		/** Adds `U` to `T`'s list of conversions.
		 * @tparam U Type that `T` can be `static_cast` to. */
		template<typename U>
		type_factory &convertible() requires good_cast<U> && unqualified<U> && different<U>
		{
			m_handle->template add_conv<T, U>();
			return *this;
		}
		/** Adds an attribute to `T`'s list of attributes.
		 * @param value Value of the attribute. */
		template<typename A>
		auto attribute(A &&value) { return attribute<std::decay_t<A>, A>(std::forward<A>(value)); }
		/** Adds an attribute to `T`'s list of attributes.
		 * @tparam A Type of the attribute.
		 * @param args Arguments used to initialize the attribute. */
		template<typename A, typename... Args>
		type_factory<T, A, Attr...> attribute(Args &&...args)
		{
			m_handle->template add_attrib<T>(type_seq<A, Attr...>, [&](A *ptr){ std::construct_at(ptr, std::forward<Args>(args)...); });
			return type_factory<T, A, Attr...>{m_handle};
		}
		/** Adds an attribute to `T`'s list of attributes.
		 * @tparam Value Value of the attribute. */
		template<auto Value>
		auto attribute() { return attribute<std::remove_cvref_t<decltype(Value)>, Value>(); }
		/** @copydoc attribute
		 * @tparam A Type of the attribute. */
		template<typename A, auto Value>
		type_factory<T, A, Attr...> attribute()
		{
			m_handle->template add_attrib<T, Value>(type_seq<A, Attr...>);
			return type_factory<T, A, Attr...>{m_handle};
		}
		// clang-format on

		/** Finalizes the type and inserts it into the type database.
		 * @note After call to `m_handle` type factory is in invalid state. */
		inline void submit();

	private:
		handle_t m_handle;
	};

	/** @brief Structure used to reference reflected information about a type. */
	class type_info
	{
		friend struct detail::type_handle;
		friend class type_database;
		friend class any;

		using data_t = detail::type_data;
		using handle_t = detail::type_handle;

		template<typename T>
		[[nodiscard]] static data_t *get_data() noexcept;
		template<typename T>
		[[nodiscard]] constexpr static handle_t get_handle() noexcept
		{
			return handle_t{type_selector<std::remove_cvref_t<T>>};
		}

	public:
		/** Returns type info for type `T`.
		 * @note Removes any const & volatile qualifiers and decays references. */
		template<typename T>
		[[nodiscard]] constexpr static type_info get()
		{
			return type_info{get_handle<T>()};
		}

		/** Searches for a reflected type in the type database.
		 * @return Type info of the type, or an invalid type info if such type is not found. */
		[[nodiscard]] inline static type_info get(std::string_view name);

		/** Reflects type `T`, making it available for runtime lookup by name.
		 * @return Type factory for type `T`, which can be used to specify additional information about the type.
		 * @note Removes any const & volatile qualifiers and decays references. */
		template<typename T>
		inline static type_factory<T> reflect();
		/** Resets a reflected type, removing it from the type database.
		 * @note The type will no longer be available for runtime lookup. */
		inline static void reset(std::string_view name);
		/** @copydoc reset */
		template<typename T>
		static void reset() noexcept
		{
			reset(type_name<std::remove_cvref_t<T>>());
		}

	private:
		friend class signature_info;
		friend class parent_info;
		friend class conversion_info;
		friend class attribute_info;

		using constructor_iterator = detail::type_node_iterator<constructor_info>;
		using function_iterator = detail::type_node_iterator<function_info>;
		using parent_iterator = detail::type_node_iterator<parent_info>;
		using conversion_iterator = detail::type_node_iterator<conversion_info>;
		using attribute_iterator = detail::type_node_iterator<attribute_info>;

		constexpr explicit type_info(const data_t *data) noexcept : m_data(data) {}
		constexpr explicit type_info(handle_t handle) noexcept : m_data(handle.get()) {}

	public:
		/** Initializes an invalid type info (type info with no underlying type). */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a reflected type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }
		/** @copydoc valid */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns the name of the underlying type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_data->name; }

		/** Checks if the underlying type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return m_data->is_empty(); }
		/** Checks if the underlying type has an extent (is a bounded array). */
		[[nodiscard]] constexpr bool has_extent() const noexcept { return m_data->has_extent(); }
		/** Checks if the underlying type is an array. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return m_data->is_array(); }
		/** Checks if the underlying type is a range. */
		[[nodiscard]] constexpr bool is_range() const noexcept { return m_data->is_range(); }
		/** Checks if the underlying type is a pointer. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return m_data->is_pointer(); }
		/** Checks if the underlying type is a pointer-like object. */
		[[nodiscard]] constexpr bool is_pointer_like() const noexcept { return m_data->is_pointer_like(); }

		/** Returns the extent of the underlying type.
		 * @note If the type is not a bounded array, extent is 0. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept { return m_data->extent; }
		/** Returns value type oof the underlying range, pointer or pointer-like type.
		 * @note If the type is not a range, pointer or pointer-like, returns identity. */
		[[nodiscard]] constexpr type_info value_type() const noexcept { return type_info{m_data->value_type}; }

		/** Returns a view containing constructors of this type. */
		[[nodiscard]] constexpr detail::type_data_view<constructor_iterator> constructors() const noexcept;
		/** Checks if the type is constructable with the specified set of arguments. */
		[[nodiscard]] constexpr bool constructable_with(std::span<type_info> args) const noexcept;
		/** @copydoc constructable_with */
		[[nodiscard]] constexpr bool constructable_with(std::span<any> args) const noexcept;
		/** Constructs the underlying type with the passed arguments & returns an `any` managing the constructed object.
		 * @param args Arguments passed to the constructor.
		 * @return `any` managing the constructed object.
		 * @throw invalid_member If no constructor accepting `args` was found.
		 * @throw bad_any_const If const-ness of the passed arguments is invalid (expected non-const but got const). */
		[[nodiscard]] SEK_API any construct(std::span<any> args = {}) const;
		// clang-format off
		/** @copydoc construct */
		template<typename... AnyArgs>
		[[nodiscard]] any construct(AnyArgs &&...args) const requires detail::any_args<AnyArgs...>;
		// clang-format on

		/** Returns a view containing functions of this type. */
		[[nodiscard]] constexpr detail::type_data_view<function_iterator> functions() const noexcept;
		/** Invokes the specified function on the passed instance.
		 * @param name Name of the reflected function.
		 * @param instance Instance of the object this function is invoked on.
		 * If `function_info` represents a static function who's first argument is not an instance pointer, instance is ignored.
		 * @param args Arguments passed to the function.
		 * @return Value returned by the function. If the underlying function's return type is void, returns an empty `any`.
		 * @throw invalid_member If such function was not found.
		 * @throw bad_any_type If the function cannot be invoked with the passed arguments.
		 * @throw bad_any_const If const-ness of the passed arguments is invalid (expected non-const but got const).
		 * @warning Invoking a non-const function on a const object will result in undefined behavior. */
		SEK_API any invoke(std::string_view name, any instance, std::span<any> args) const;
		// clang-format off
		/** @copydoc invoke */
		template<typename... AnyArgs>
		any invoke(std::string_view name, any instance, AnyArgs &&...args) const requires detail::any_args<AnyArgs...>;
		// clang-format on

		/** Returns a view containing parents of this type. */
		[[nodiscard]] constexpr detail::type_data_view<parent_iterator> parents() const noexcept;
		/** Checks if the underlying type inherits a type with the specified name. */
		[[nodiscard]] constexpr bool inherits(std::string_view name) const noexcept
		{
			const auto pred = [name](auto &n) { return n.type->name == name || type_info{n.type}.inherits(name); };
			return std::ranges::any_of(m_data->parents, pred);
		}
		/** Checks if the underlying type inherits the specified type. */
		[[nodiscard]] constexpr bool inherits(type_info info) const noexcept { return inherits(info.name()); }
		/** Checks if the underlying type inherits 'T'. */
		template<typename T>
		[[nodiscard]] constexpr bool inherits() const noexcept
		{
			return inherits(type_name<std::remove_cvref_t<T>>());
		}

		/** Returns a view containing attributes of this type. */
		[[nodiscard]] constexpr detail::type_data_view<attribute_iterator> attributes() const noexcept;
		/** Checks if the type has an attribute of a type with the specified name. */
		[[nodiscard]] constexpr bool has_attribute(std::string_view name) const noexcept
		{
			const auto pred = [name](auto &n) { return n.type->name == name; };
			return std::ranges::any_of(m_data->attribs, pred);
		}
		/** Checks if the type has an attribute of the specified type. */
		[[nodiscard]] constexpr bool has_attribute(type_info info) const noexcept { return has_attribute(info.name()); }
		/** Checks if the type has an attribute of type `T`. */
		template<typename T>
		[[nodiscard]] constexpr bool has_attribute() const noexcept
		{
			return has_attribute(type_name<std::remove_cvref_t<T>>());
		}
		/** Returns any reference to attribute of a type with the specified name.
		 * @return `any` reference to type's data or an empty `any` if such attribute is not found. */
		[[nodiscard]] SEK_API any get_attribute(std::string_view name) const noexcept;
		/** Returns any reference to attribute of the specified type.
		 * @return `any` reference to type's data or an empty `any` if such attribute is not found. */
		[[nodiscard]] SEK_API any get_attribute(type_info info) const noexcept;
		/** Returns any reference to attribute of type `T`.
		 * @return `any` reference to type's data or an empty `any` if such attribute is not found. */
		template<typename T>
		[[nodiscard]] any get_attribute() const noexcept;

		/** Returns a view containing conversions of this type. */
		[[nodiscard]] constexpr detail::type_data_view<conversion_iterator> conversions() const noexcept;
		/** Checks if the underlying type is convertible to a type with the specified name via `static_cast`. */
		[[nodiscard]] constexpr bool convertible_to(std::string_view name) const noexcept
		{
			const auto pred = [name](auto &n) { return n.type->name == name; };
			return std::ranges::any_of(m_data->convs, pred);
		}
		/** Checks if the underlying type is convertible to the specified type via `static_cast`. */
		[[nodiscard]] constexpr bool convertible_to(type_info info) const noexcept
		{
			return convertible_to(info.name());
		}
		/** Checks if the underlying type is convertible to 'T' via `static_cast`. */
		template<typename T>
		[[nodiscard]] constexpr bool convertible_to() const noexcept
		{
			return convertible_to(type_name<std::remove_cvref_t<T>>());
		}

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			return m_data == other.m_data || (m_data && other.m_data && name() == other.name());
		}

		constexpr void swap(type_info &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(type_info &a, type_info &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data = nullptr;
	};

	[[nodiscard]] constexpr hash_t hash(const type_info &info) noexcept
	{
		const auto name = info.name();
		return fnv1a(name.data(), name.size());
	}

	/** @brief Service used to store a database of reflected type information. */
	class type_database : public service<shared_guard<type_database>>
	{
		friend shared_guard<type_database>;
		template<typename, typename...>
		friend class type_factory;
		friend class type_query;

		using data_t = dense_map<std::string_view, detail::type_handle>;
		using attributes_t = dense_map<std::string_view, data_t>;

		[[nodiscard]] constexpr static type_info to_info(detail::type_handle handle) noexcept
		{
			return type_info{handle};
		}

		class type_iterator;
		class type_pointer
		{
			friend class type_iterator;

			constexpr explicit type_pointer(detail::type_handle handle) noexcept : m_info(to_info(handle)) {}

		public:
			constexpr type_pointer() noexcept = default;

			[[nodiscard]] constexpr const type_info *get() const noexcept { return &m_info; }
			[[nodiscard]] constexpr const type_info *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr type_info operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr bool operator==(const type_pointer &) const noexcept = default;

		private:
			type_info m_info = {};
		};

		class type_iterator
		{
			friend class type_database;

			using iter_t = typename data_t::const_iterator;

		public:
			typedef type_info value_type;
			typedef type_pointer pointer;
			typedef type_info reference;
			typedef typename iter_t::size_type size_type;
			typedef typename iter_t::difference_type difference_type;
			typedef typename iter_t::iterator_category iterator_category;

		private:
			constexpr explicit type_iterator(iter_t iter) noexcept : m_iter(iter) {}

		public:
			constexpr type_iterator() noexcept = default;

			constexpr type_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr type_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}
			constexpr type_iterator &operator+=(difference_type n) noexcept
			{
				m_iter += n;
				return *this;
			}
			constexpr type_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr type_iterator &operator--() noexcept
			{
				--m_iter;
				return *this;
			}
			constexpr type_iterator &operator-=(difference_type n) noexcept
			{
				m_iter -= n;
				return *this;
			}

			constexpr type_iterator operator+(difference_type n) const noexcept { return type_iterator{m_iter + n}; }
			constexpr type_iterator operator-(difference_type n) const noexcept { return type_iterator{m_iter - n}; }
			constexpr difference_type operator-(const type_iterator &other) const noexcept
			{
				return m_iter - other.m_iter;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{m_iter->second}; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept
			{
				return *pointer{m_iter[n].second};
			}

			[[nodiscard]] constexpr auto operator<=>(const type_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const type_iterator &) const noexcept = default;

			constexpr void swap(type_iterator &other) noexcept { std::swap(m_iter, other.m_iter); }
			friend constexpr void swap(type_iterator &a, type_iterator &b) noexcept { a.swap(b); }

		private:
			iter_t m_iter;
		};

	public:
		typedef type_info value_type;
		typedef type_pointer pointer;
		typedef type_pointer const_pointer;
		typedef type_info reference;
		typedef type_info const_reference;
		typedef type_iterator iterator;
		typedef type_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef typename data_t::size_type size_type;
		typedef typename data_t::difference_type difference_type;

	public:
		/** Returns iterator to the first type of the database. */
		[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{m_types.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator{m_types.cbegin()}; }
		/** Returns iterator one past the last type of the database. */
		[[nodiscard]] constexpr iterator end() const noexcept { return iterator{m_types.end()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator{m_types.cend()}; }
		/** Returns reverse iterator to the last type of the database. */
		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return crbegin(); }
		/** Returns iterator one past the first type of the database. */
		[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns size of the database (total amount of reflected types). */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_types.size(); }
		/** Checks if the type database is empty (no types are reflected). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_types.empty(); }

		/** Returns a type query for the database. */
		[[nodiscard]] constexpr type_query query() const noexcept;

		/** Searches for a reflected type in the database.
		 * @return Iterator to the taraget type, or end iterator if the type was not found. */
		[[nodiscard]] SEK_API iterator find(std::string_view name) const;
		/** Searches for a reflected type in the database.
		 * @return Type info of the type, or an invalid type info if such type is not found. */
		[[nodiscard]] SEK_API type_info get(std::string_view name) const;

		/** Reflects type `T`, making it available for runtime lookup by name.
		 * @return Type factory for type `T`, which can be used to specify additional information about the type.
		 * @note Removes any const & volatile qualifiers and decays references. */
		template<typename T>
		constexpr type_factory<T> reflect()
		{
			return type_factory<T>{type_info::get_handle<T>()};
		}

		/** Resets a reflected type, removing it from the database.
		 * @note The type will no longer be available for runtime lookup. */
		SEK_API void reset(std::string_view name);
		/** @copydoc reset */
		SEK_API void reset(const_iterator which);
		/** @copydoc reset */
		template<typename T>
		void reset()
		{
			reset(type_name<T>());
		}

	private:
		SEK_API void reflect_impl(detail::type_handle);

		data_t m_types;
		attributes_t m_attributes;
	};

	template<typename T, typename... Attr>
	void type_factory<T, Attr...>::submit()
	{
		if (m_handle.get != nullptr) [[likely]]
		{
			type_database::instance()->access_unique()->reflect_impl(m_handle);
			m_handle = {};
		}
	}

	template<typename T>
	type_factory<T> type_info::reflect()
	{
		return type_database::instance()->access_unique()->template reflect<T>();
	}
	type_info type_info::get(std::string_view name) { return type_database::instance()->access_shared()->get(name); }
	void type_info::reset(std::string_view name) { return type_database::instance()->access_unique()->reset(name); }

	/** @brief Structure used to query type database for a set of types. */
	class type_query
	{
		using data_t = dense_set<type_info>;

	public:
		typedef typename data_t::value_type value_type;
		typedef typename data_t::pointer pointer;
		typedef typename data_t::const_pointer const_pointer;
		typedef typename data_t::reference reference;
		typedef typename data_t::const_reference const_reference;
		typedef typename data_t::iterator iterator;
		typedef typename data_t::const_iterator const_iterator;
		typedef typename data_t::reverse_iterator reverse_iterator;
		typedef typename data_t::const_reverse_iterator const_reverse_iterator;
		typedef typename data_t::size_type size_type;
		typedef typename data_t::difference_type difference_type;

	public:
		/** Initializes type query for the specified type database. */
		constexpr type_query(const type_database &db) noexcept : m_db(db)
		{
			m_types.reserve(m_db.size());
			for (auto type : m_db) m_types.insert(type);
		}

		/** Returns iterator to the first type captured by the query. */
		[[nodiscard]] constexpr iterator begin() const noexcept { return m_types.begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return m_types.cbegin(); }
		/** Returns iterator one past the last type captured by the query. */
		[[nodiscard]] constexpr iterator end() const noexcept { return m_types.end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return m_types.cend(); }
		/** Returns reverse iterator to the last type captured by the query. */
		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return m_types.rbegin(); }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return m_types.crbegin(); }
		/** Returns iterator one past the first type captured by the query. */
		[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return m_types.rend(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return m_types.crend(); }

		/** Returns the number of types captured by the query. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_types.size(); }
		/** Checks if the query is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_types.empty(); }

		/** Captures all types for which the passed predicate evaluates to `true`.
		 * @param pred Predicate to use.
		 * @return Reference to the query. */
		template<typename P>
		constexpr type_query &matching(P pred)
		{
			for (auto type = end(); type-- != begin();)
				if (!pred(*type)) type = m_types.erase(type);
			return *this;
		}

		/** Captures all types with the specified attribute. */
		constexpr type_query &with_attribute(std::string_view name)
		{
			if (auto attrib = m_db.m_attributes.find(name); attrib != m_db.m_attributes.end()) [[likely]]
			{
				for (auto type = end(); type-- != begin();)
					if (!attrib->second.contains(type->name())) type = m_types.erase(type);
			}
			return *this;
		}
		/** @copydoc with_attribute */
		constexpr type_query &with_attribute(type_info type) { return with_attribute(type.name()); }
		/** @copydoc with_attribute */
		template<typename A>
		constexpr type_query &with_attribute()
		{
			return with_attribute(type_name<A>());
		}

		/** Captures all types with all of the specified attributes. */
		template<typename... As>
		constexpr type_query &with_attributes()
		{
			auto attribs = std::forward_as_tuple(m_db.m_attributes.find(type_name<As>())...);
			return with_attributes(std::make_index_sequence<sizeof...(As)>{}, attribs);
		}

	private:
		template<size_type I>
		[[nodiscard]] constexpr bool check_attrib(const_iterator type, auto &attribs)
		{
			if (auto attrib = std::get<I>(attribs); attrib != m_db.m_attributes.end()) [[likely]]
				return attrib->second.contains(type->name());
		}
		template<size_type... Is>
		constexpr type_query &with_attributes(std::index_sequence<Is...>, auto &attribs)
		{
			for (auto type = end(); type-- != begin();)
				if (!(check_attrib<Is>(type, attribs) && ...)) type = m_types.erase(type);
			return *this;
		}

		const type_database &m_db;
		data_t m_types;
	};

	constexpr type_query type_database::query() const noexcept { return type_query{*this}; }

	template<typename T>
	type_info::data_t *type_info::get_data() noexcept
	{
		constinit static auto data = detail::make_type_data<T>();
		return &data;
	}
	template<typename T>
	constexpr detail::type_handle::type_handle(type_selector_t<T>) noexcept : get(type_info::get_data<T>)
	{
	}
	constexpr bool detail::type_handle::operator==(const detail::type_handle &other) const noexcept
	{
		auto p = get(), other_p = other.get();
		return p == other_p || p->name == other_p->name;
	}

	/** @brief Type-erased container of objects. */
	class any
	{
		friend class any_ref;

		struct vtable_t
		{
			void (*copy_construct)(any &, const any &);
			void (*copy_assign)(any &, const any &);
			bool (*compare)(const any &, const any &);
			void (*destroy)(any &);
		};

		template<typename T>
		constexpr static bool local_candidate = sizeof(T) <= sizeof(std::intptr_t) && std::is_trivially_copyable_v<T>;

		union storage_t
		{
			constexpr storage_t() noexcept = default;

			template<typename T>
			constexpr storage_t(T *ptr) noexcept : external(const_cast<std::remove_const_t<T> *>(ptr))
			{
			}

			template<typename T, typename... Args>
			constexpr storage_t(std::in_place_type_t<T>, Args &&...args) noexcept
			{
				init<T>(std::forward<Args>(args)...);
			}

			// clang-format off
			template<typename T, typename... Args>
			constexpr void init(Args &&...) requires std::is_void_v<T> {}
			template<typename T, typename... Args>
			constexpr void init(Args &&...args) requires local_candidate<T>
			{
				if constexpr (sizeof...(Args) == 0 || std::is_aggregate_v<T>)
					new (local.data()) T{std::forward<Args>(args)...};
				else
					new (local.data()) T(std::forward<Args>(args)...);
			}
			template<typename T, typename... Args>
			constexpr void init(Args &&...args)
			{
				using U = std::remove_const_t<T>;
				if constexpr (sizeof...(Args) == 0 || std::is_aggregate_v<T>)
					external = const_cast<U *>(new T{std::forward<Args>(args)...});
				else
					external = const_cast<U *>(new T(std::forward<Args>(args)...));
			}
			// clang-format on

			constexpr void swap(storage_t &other) noexcept { std::swap(local, other.local); }

			type_storage<std::intptr_t> local = {};
			void *external;
		};

		enum flags_t
		{
			NO_FLAGS = 0,
			IS_REF = 1,
			IS_LOCAL = 2,
			IS_CONST = 4,
		};

		template<typename T>
		struct vtable_instance
		{
			constinit const static vtable_t value;
		};

		template<typename T>
		[[nodiscard]] constexpr static flags_t make_flags() noexcept
		{
			const flags_t result = std::is_const_v<std::remove_reference_t<T>> ? IS_CONST : NO_FLAGS;
			if constexpr (std::is_lvalue_reference_v<T>)
				return static_cast<flags_t>(result | IS_REF);
			else if constexpr (local_candidate<T>)
				return static_cast<flags_t>(result | IS_LOCAL);
			else
				return result;
		}

		constexpr any(const vtable_t *vtable, type_info info, storage_t storage, flags_t flags) noexcept
			: m_vtable(vtable), m_info(info), m_storage(storage), m_flags(flags)
		{
		}

	public:
		/** Initializes an empty instance of `any`. */
		constexpr any() noexcept = default;
		constexpr any(any &&other) noexcept { swap(other); }
		constexpr any &operator=(any &&other) noexcept
		{
			swap(other);
			return *this;
		}

		any(const any &other) { copy_construct(other); }
		any &operator=(const any &other)
		{
			if (this != &other) [[likely]]
				copy_assign(other);
			return *this;
		}
		~any() { reset_impl(); }

		/** Initializes `any` with the managed object direct-initialized from `value`. */
		// clang-format off
		template<typename T>
		any(T &&value) noexcept requires(!std::same_as<std::decay_t<T>, any>) : any(std::in_place_type<T>, std::forward<T>(value))
		{
		}
		// clang-format on

		/** Initializes an empty `any`. */
		constexpr explicit any(std::in_place_type_t<void>) noexcept : any{} {}

		/** Initializes `any` in-place using the passed arguments. */
		template<typename T, typename... Args>
		explicit any(std::in_place_type_t<T>, Args &&...args) noexcept
			: any(&vtable_instance<std::decay_t<T>>::value,
				  type_info::get<T>(),
				  storage_t(std::in_place_type<std::decay_t<T>>, std::forward<Args>(args)...),
				  make_flags<std::decay_t<T>>())
		{
		}
		/** @copydoc any */
		template<typename T, typename U, typename... Args>
		explicit any(std::in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) noexcept
			: any(std::in_place_type<T>, std::forward<std::initializer_list<U>>(il), std::forward<Args>(args)...)
		{
		}

		/** Initializes `any` to reference an externally-stored object. */
		template<typename T>
		constexpr explicit any(std::in_place_type_t<T &>, T &ref) noexcept
			: any(&vtable_instance<T>::value, type_info::get<T>(), std::addressof(ref), make_flags<T &>())
		{
		}

		/** Returns type info of the managed object. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_info; }

		/** Checks if `any` manages an object. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_vtable == nullptr; }
		/** Checks if `any` references an externally-stored object. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return m_flags & IS_REF; }
		/** Checks if the managed object is stored in-place. */
		[[nodiscard]] constexpr bool is_local() const noexcept { return m_flags & IS_LOCAL; }
		/** Checks if the managed object is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_flags & IS_CONST; }

		/** Resets `any` by destroying and releasing the internal object. */
		void reset()
		{
			reset_impl();
			m_vtable = nullptr;
			m_info = {};
			m_storage = {};
			m_flags = {};
		}

		/** Returns raw pointer to the managed object's data.
		 * @note If the managed object is const-qualified, returns nullptr. */
		[[nodiscard]] constexpr void *data() noexcept
		{
			return is_const() ? nullptr : is_local() ? m_storage.local.data() : m_storage.external;
		}
		/** Returns raw const pointer to the managed object's data. */
		[[nodiscard]] constexpr const void *cdata() const noexcept
		{
			return is_local() ? m_storage.local.data() : m_storage.external;
		}
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return cdata(); }

		/** Returns an `any` referencing to the managed object.
		 * @note Preserves const-ness of the managed object. */
		[[nodiscard]] any ref() noexcept
		{
			return any{m_vtable, m_info, cdata(), static_cast<flags_t>(IS_REF | (m_flags & IS_CONST))};
		}
		/** Returns a const `any` referencing to the managed object. */
		[[nodiscard]] any cref() const noexcept
		{
			return any{m_vtable, m_info, cdata(), static_cast<flags_t>(IS_REF | IS_CONST)};
		}
		/** @copydoc cref */
		[[nodiscard]] any ref() const noexcept { return cref(); }

		/** Returns pointer to the managed object as `T` pointer.
		 * @note `T` must be the same as the underlying object.
		 * @return Pointer to the underlying object or nullptr if the underlying object is const or of a different type. */
		template<typename T>
		[[nodiscard]] constexpr T *as_ptr() noexcept
		{
			if constexpr (std::is_const_v<T>)
				return as_cptr<T>();
			else
			{
				if (m_info == type_info::get<T>()) [[likely]]
					return static_cast<T *>(data());
				return nullptr;
			}
		}
		/** Returns const pointer to the managed object as `T` pointer.
		 * @note `T` must be the same as the underlying object.
		 * @return Pointer to the underlying object or nullptr if the underlying object is of a different type. */
		template<typename T>
		[[nodiscard]] constexpr std::add_const_t<T> *as_cptr() const noexcept
		{
			using U = std::remove_const_t<T>;
			if (m_info == type_info::get<U>()) [[likely]]
				return static_cast<const U *>(cdata());
			return nullptr;
		}
		/** @copydoc as_ptr */
		template<typename T>
		[[nodiscard]] constexpr std::add_const_t<T> *as_ptr() const noexcept
		{
			return as_cptr<T>();
		}

		/** @brief Attempts to cast the underlying object to type `T`.
		 *
		 * If the type of the managed object is same as `std::remove_cvref_t<T>`, equivalent to `as_ptr<T>()`.
		 * Otherwise, attempts to cast the underlying object using one of it's reflected parents.
		 *
		 * @return Pointer to the underlying object, cast to type `T`, or nullptr if such cast is not available. */
		template<typename T>
		[[nodiscard]] T *try_cast() noexcept;
		/** @copydoc try_cast */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *try_cast() const noexcept;
		/** @brief Casts the underlying object to type `T`.
		 *
		 * If the type of the managed object is same as `std::remove_cvref_t<T>`,
		 * equivalent to `*as_ptr<std::remove_ref_t<T>>()`. Otherwise, attempts to cast
		 * the underlying object using one of it's reflected parents.
		 *
		 * @return The underlying underlying object, cast to type `T`.
		 *
		 * @throw bad_any_type If no such cast is possible. */
		template<typename T>
		[[nodiscard]] T cast()
		{
			using U = std::remove_reference_t<T>;
			if constexpr (std::same_as<U, T>) /* Always treat by-value casts as const. */
				return static_cast<const any *>(this)->template cast<T>();
			else
			{
				auto ptr = try_cast<U>();
				if (!ptr) [[unlikely]]
				{
					std::string msg = "Invalid any cast to type \"";
					msg.append(type_name<std::remove_cvref_t<T>>());
					msg.append(1, '\"');
					throw bad_any_type(std::move(msg));
				}
				return static_cast<T>(*ptr);
			}
		}
		/** @copydoc cast */
		template<typename T>
		[[nodiscard]] T cast() const
		{
			auto ptr = try_cast<std::remove_reference_t<T>>();
			if (!ptr) [[unlikely]]
			{
				std::string msg = "Invalid any cast to type \"";
				msg.append(type_name<std::remove_cvref_t<T>>());
				msg.append(1, '\"');
				throw bad_any_type(std::move(msg));
			}
			return static_cast<T>(*ptr);
		}

		/** @brief Converts the underlying object to the passed type.
		 *
		 * If the type of the managed object is same as `type`,
		 * equivalent to `ref()`. Otherwise, attempts to cast the underlying
		 * object using one of it's reflected parents and explicit conversions.
		 *
		 * @param to_type Type to convert the managed object to.
		 * @return An instance of `any` containing the converted instance, or empty `any` if no such conversion is possible.
		 * @note If no parent cast is found, explicit conversion will return a copy of the underlying object. */
		[[nodiscard]] SEK_API any convert(std::string_view to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] SEK_API any convert(type_info to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] SEK_API any convert(std::string_view to_type) const noexcept;
		/** @copydoc convert */
		[[nodiscard]] SEK_API any convert(type_info to_type) const noexcept;

		/** Invokes the specified function on the managed object.
		 * @param name Name of the reflected function.
		 * @param args Arguments passed to the function.
		 * @return Value returned by the function. If the underlying function's return type is void, returns an empty `any`.
		 * @throw invalid_member If such function was not found.
		 * @throw bad_any_type If the function cannot be invoked with the passed arguments.
		 * @throw bad_any_const If const-ness of the passed arguments is invalid (expected non-const but got const).
		 * @warning Invoking a non-const function on a const object will result in undefined behavior. */
		SEK_API any invoke(std::string_view name, std::span<any> args);
		/** @copydoc invoke */
		SEK_API any invoke(std::string_view name, std::span<any> args) const;
		// clang-format off
		/** @copydoc invoke */
		template<typename... AnyArgs>
		any invoke(std::string_view name, AnyArgs &&...args) requires detail::any_args<AnyArgs...>;
		/** @copydoc invoke */
		template<typename... AnyArgs>
		any invoke(std::string_view name, AnyArgs &&...args) const requires detail::any_args<AnyArgs...>;
		// clang-format on

		/** Compares managed objects by-value.
		 * @return true if the managed objects compare equal,
		 * false if they do not or if they are of different types. */
		[[nodiscard]] bool operator==(const any &other) const
		{
			if (empty() && other.empty()) [[unlikely]]
				return true;
			else
				return m_info == other.m_info && m_vtable->compare(*this, other);
		}

		constexpr void swap(any &other) noexcept
		{
			std::swap(m_vtable, other.m_vtable);
			m_info.swap(other.m_info);
			m_storage.swap(other.m_storage);
			std::swap(m_flags, other.m_flags);
		}
		friend constexpr void swap(any &a, any &b) noexcept { a.swap(b); }

	private:
		void reset_impl()
		{
			/* References are not destroyed. */
			if (!(m_flags & IS_REF) && m_vtable != nullptr) [[likely]]
				m_vtable->destroy(*this);
		}
		void copy_construct(const any &from)
		{
			if (from.m_vtable != nullptr) [[likely]]
				from.m_vtable->copy_construct(*this, from);
			m_vtable = from.m_vtable;
			m_info = from.m_info;
		}
		void copy_assign(const any &from)
		{
			if (empty() && from.m_vtable != nullptr) [[unlikely]]
				from.m_vtable->copy_construct(*this, from);
			else if (from.m_vtable != nullptr) [[likely]]
				from.m_vtable->copy_assign(*this, from);
			else
				reset();
			m_vtable = from.m_vtable;
			m_info = from.m_info;
		}

		const vtable_t *m_vtable = nullptr;
		type_info m_info;
		storage_t m_storage = {};
		flags_t m_flags = {};
	};

	template<typename T>
	constinit const any::vtable_t any::vtable_instance<T>::value = {
		.copy_construct = +[](any &to, const any &from) -> void
		{
			to.m_storage.template init<T>(*static_cast<const T *>(from.data()));
			to.m_flags = make_flags<T>();
		},
		.copy_assign = +[](any &to, const any &from) -> void
		{
			constexpr auto reset_copy = [](any &t, const any &f)
			{
				t.reset();
				t.m_storage.template init<T>(*static_cast<const T *>(f.data()));
			};

			using U = std::add_const_t<T>;
			if constexpr (!requires(T a, U b) { a = b; })
				reset_copy(to, from);
			else
			{
				if (to.m_info != from.m_info)
					reset_copy(to, from);
				else if constexpr (local_candidate<T>)
					*to.m_storage.local.template get<T>() = *static_cast<U *>(from.data());
				else
					*static_cast<T *>(to.m_storage.external) = *static_cast<U *>(from.data());
			}
			to.m_flags = make_flags<T>();
		},
		.compare = +[](const any &a, const any &b) -> bool
		{
			/* At this point, both types are equal. */
			const auto a_ptr = a.template as_cptr<T>();
			const auto b_ptr = b.template as_cptr<T>();
			if (a_ptr == b_ptr) [[unlikely]]
				return true;
			else if constexpr (!std::equality_comparable<T>)
				return false;
			else
				return *a_ptr == *b_ptr;
		},
		.destroy = +[](any &instance) -> void
		{
			if constexpr (local_candidate<T>)
				std::destroy_at(instance.m_storage.local.template get<T>());
			else if constexpr (std::is_bounded_array_v<T>)
				delete[] static_cast<T *>(instance.m_storage.external);
			else
				delete static_cast<T *>(instance.m_storage.external);
		},
	};

	/** Forwards the passed value by-reference if possible, otherwise constructs a new instance in-place. */
	template<typename T>
	[[nodiscard]] any forward_any(T &&value)
	{
		using U = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::decay_t<T>>;
		return any{std::in_place_type<U>, value};
	}

	/** Returns `any`, containing an instance of `T` constructed in-place.
	 * @param args Arguments used to construct the instance. */
	template<typename T, typename... Args>
	[[nodiscard]] any make_any(Args &&...args)
	{
		using U = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::decay_t<T>>;
		return any{std::in_place_type<U>, std::forward<Args>(args)...};
	}
	/** @copydoc make_any
	 * @param il Initializer list passed to the constructor. */
	template<typename T, typename U, typename... Args>
	[[nodiscard]] any make_any(std::initializer_list<U> il, Args &&...args)
	{
		return any{std::in_place_type<std::decay_t<T>>, std::forward<std::initializer_list<U>>(il), std::forward<Args>(args)...};
	}

	/** @brief Type-erased reference to objects. Effectively it is a reference-only wrapper around `any`. */
	class any_ref
	{
	public:
		any_ref() = delete;
		any_ref(const any_ref &) = delete;
		any_ref &operator=(const any_ref &) = delete;

		/** Initializes `any` reference from an `any` instance by-move.
		 * @param data `any` storing a reference to an object.
		 * @note Provided `any` must be a reference, using a non-reference `any` will result in undefined behavior. */
		any_ref(any &&data) noexcept
		{
			SEK_ASSERT(data.is_ref(), "Unable to move-initialize `any_ref` from a non-reference `any`");
			std::construct_at(m_storage.get<any>(), std::move(data));
		}
		/** Initializes `any` reference from an `any` instance.
		 * @param data `any` containing data to be referenced. */
		any_ref(any &data) noexcept : any_ref(data.ref()) {}
		/** @copydoc any_ref */
		any_ref(const any &data) noexcept : any_ref(data.ref()) {}

		constexpr any_ref(any_ref &&other) noexcept
		{
			std::construct_at(m_storage.get<any>(), std::move(other.value()));
		}
		constexpr any_ref &operator=(any_ref &&other) noexcept
		{
			value().swap(other.value());
			return *this;
		}
		constexpr ~any_ref() = default;

		/** @copydoc any::type */
		[[nodiscard]] constexpr type_info type() const noexcept { return value().type(); }

		/** @copydoc any::empty */
		[[nodiscard]] constexpr bool empty() const noexcept { return value().empty(); }
		/** @copydoc any::is_const */
		[[nodiscard]] constexpr bool is_const() const noexcept { return value().is_const(); }

		/** @copydoc any::data */
		[[nodiscard]] constexpr void *data() noexcept { return !is_const() ? value().m_storage.external : nullptr; }
		/** @copydoc any::cdata */
		[[nodiscard]] constexpr const void *cdata() const noexcept { return value().m_storage.external; }
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return value().m_storage.external; }

		/** @copydoc any::as_ptr */
		template<typename T>
		[[nodiscard]] constexpr T *as_ptr() noexcept
		{
			if constexpr (std::is_const_v<T>)
				return as_cptr<T>();
			else
			{
				if (type() == type_info::get<T>()) [[likely]]
					return static_cast<T *>(data());
				return nullptr;
			}
		}
		/** @copydoc any::as_cptr */
		template<typename T>
		[[nodiscard]] constexpr std::add_const_t<T> *as_cptr() const noexcept
		{
			using U = std::remove_const_t<T>;
			if (type() == type_info::get<U>()) [[likely]]
				return static_cast<const U *>(cdata());
			return nullptr;
		}
		/** @copydoc as_ptr */
		template<typename T>
		[[nodiscard]] constexpr std::add_const_t<T> *as_ptr() const noexcept
		{
			return as_cptr<T>();
		}

		/** @copydoc any::try_cast */
		template<typename T>
		[[nodiscard]] T *try_cast() noexcept;
		/** @copydoc try_cast */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *try_cast() const noexcept;
		/** @copydoc any::cast */
		template<typename T>
		[[nodiscard]] T cast();
		/** @copydoc cast */
		template<typename T>
		[[nodiscard]] T cast() const;

		/** @brief Converts the underlying object to the passed type.
		 *
		 * If the type of the managed object is same as `type`,
		 * equivalent to `operator any()`. Otherwise, attempts to cast the underlying
		 * object using one of it's reflected parents and explicit conversions.
		 *
		 * @param to_type Type to convert the managed object to.
		 * @return An instance of `any` containing the converted instance, or empty `any` if no such conversion is possible.
		 * @note If no parent cast is found, explicit conversion will return a copy of the underlying object. */
		[[nodiscard]] SEK_API any convert(std::string_view to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] SEK_API any convert(type_info to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] SEK_API any convert(std::string_view to_type) const noexcept;
		/** @copydoc convert */
		[[nodiscard]] SEK_API any convert(type_info to_type) const noexcept;

		/** @copydoc any::invoke */
		SEK_API any invoke(std::string_view name, std::span<any> args);
		/** @copydoc any::invoke */
		SEK_API any invoke(std::string_view name, std::span<any> args) const;
		// clang-format off
		/** @copydoc any::invoke */
		template<typename... AnyArgs>
		any invoke(std::string_view name, AnyArgs &&...args) requires detail::any_args<AnyArgs...>;
		/** @copydoc any::invoke */
		template<typename... AnyArgs>
		any invoke(std::string_view name, AnyArgs &&...args) const requires detail::any_args<AnyArgs...>;
		// clang-format on

		/** Returns `any` isntance referencing the managed object. */
		[[nodiscard]] operator any() noexcept { return value().ref(); }
		/** @copydoc operator any */
		[[nodiscard]] operator any() const noexcept { return value().ref(); }

		/** @copydoc any::operator== */
		[[nodiscard]] bool operator==(const any &other) const { return value() == other; }
		/** @copydoc any::operator== */
		[[nodiscard]] bool operator==(const any_ref &other) const { return value() == other.value(); }

		constexpr void swap(any_ref &other) noexcept { value().swap(other.value()); }
		friend constexpr void swap(any_ref &a, any_ref &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr any &value() noexcept { return *m_storage.get<any>(); }
		[[nodiscard]] constexpr const any &value() const noexcept { return *m_storage.get<any>(); }

		type_storage<any> m_storage;
	};

	namespace detail
	{
		template<typename T, typename A, typename... As>
		struct type_data::attrib_instance<T, type_seq_t<A, As...>> : attrib_node
		{
			constexpr attrib_instance() : attrib_node(type_handle{type_selector<A>}) {}

			constexpr void init(auto &&initializer)
			{
				if (destroy) [[unlikely]] /* If already initialized, destroy the old instance. */
					destroy(this);

				initializer(data.template get<A>());
				attrib_node::get_any = +[](const attrib_node *n) -> any_ref
				{
					auto data_ptr = static_cast<const attrib_instance *>(n)->data.template get<A>();
					return any_ref{forward_any(*data_ptr)};
				};
				attrib_node::destroy = +[](attrib_node *n) -> void
				{
					auto data_ptr = static_cast<attrib_instance *>(n)->data.template get<A>();
					std::destroy_at(data_ptr);
				};
			}

			type_storage<A> data = {};
		};
		template<typename T, typename A, typename... As, auto V>
		struct type_data::attrib_instance<T, type_seq_t<A, As...>, V> : attrib_node
		{
			constexpr attrib_instance() : attrib_node(type_handle{type_selector<A>})
			{
				attrib_node::get_any = +[](const attrib_node *) -> any_ref
				{
					constexpr auto &ref = auto_constant<V>::value;
					return any_ref{forward_any<std::add_const_t<A> &>(ref)};
				};
			}
		};

		SEK_API void assert_mutable_any(const any &a, std::string_view name);
		template<typename T>
		constexpr decltype(auto) unwrap_any_args(type_selector_t<T>, any &a)
		{
			using U = std::remove_reference_t<T>;
			if constexpr (!std::is_reference_v<T> || std::is_const_v<U>)
				return *a.as_cptr<U>();
			else
			{
				assert_mutable_any(a, type_name<std::remove_cvref_t<T>>());
				return *a.as_ptr<U>();
			}
		}

		template<typename T, typename... Args>
		constinit type_data::ctor_instance<T, Args...> type_data::ctor_instance<T, Args...>::value = {
			{type_handle{type_selector<std::decay_t<Args>>}...},
			+[](std::span<any> args) -> any
			{
				constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, std::span<any> & as)
				{
					return make_any<T>(unwrap_any_args(type_seq_selector<Is, type_seq_t<Args...>>, as[Is])...);
				};
				return unwrap(std::make_index_sequence<sizeof...(Args)>(), args);
			},
		};

		template<typename T, typename... Args>
		void type_data::add_ctor() noexcept
		{
			if constexpr (sizeof...(Args) != 0) /* Default constructor is automatically added. */
			{
				/* We want to preserve all qualifiers of arguments,
				 * as they will be used for binding. */
				auto &node = ctor_instance<T, Args...>::value;
				if (!node.next) [[likely]]
					constructors.insert(node);
			}
		}
		template<typename T, auto F, typename R, typename... Args>
		void type_data::add_func(type_seq_t<Args...>, std::string_view func_name) noexcept
		{
			constinit static func_instance<T, F, R, Args...> node{
				type_handle{type_selector<std::decay_t<R>>},
				{type_handle{type_selector<std::decay_t<Args>>}...},
				+[](any instance, std::span<any> args) -> any
				{
					using func_type = std::remove_const_t<decltype(F)>;
					using traits = func_traits<F>;

					// clang-format off
					constexpr bool ignore_return = std::is_void_v<R>;
					constexpr bool is_member = requires { typename traits::instance_type; };
					constexpr bool is_const_member = std::conditional_t<is_member, std::is_const<typename traits::instance_type>, std::false_type>::value;
					constexpr bool is_member_like = !is_member && std::is_invocable_r_v<R, func_type, T *, Args...>;
					constexpr bool is_const_member_like = !is_member && std::is_invocable_r_v<R, func_type, std::add_const_t<T> *, Args...>;
					// clang-format on

					if constexpr (is_const_member)
					{
						// clang-format off
						constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, any_ref i, std::span<any> &a) -> decltype(auto)
						{
							const auto *ptr = i.template as_ptr<std::add_const_t<T>>();
							return (ptr->*F)(unwrap_any_args(type_seq_selector<Is, type_seq_t<Args...>>, a[Is])...);
						};
						// clang-format on
						if constexpr (ignore_return)
						{
							unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args);
							return any{};
						}
						else
							return forward_any(unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args));
					}
					else if constexpr (is_member)
					{
						// clang-format off
						constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, any_ref i, std::span<any> &a) -> decltype(auto)
						{
							assert_mutable_any(i, type_name<std::remove_cvref_t<T>>());
							auto *ptr = i.template as_ptr<T>();
							return (ptr->*F)(unwrap_any_args(type_seq_selector<Is, type_seq_t<Args...>>, a[Is])...);
						};
						// clang-format on
						if constexpr (ignore_return)
						{
							unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args);
							return any{};
						}
						else
							return forward_any(unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args));
					}
					else if constexpr (is_const_member_like)
					{
						// clang-format off
						constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, any_ref i, std::span<any> &a) -> decltype(auto)
						{
							const auto *ptr = i.template as_ptr<std::add_const_t<T>>();
							return F(ptr, unwrap_any_args(type_seq_selector<Is, type_seq_t<Args...>>, a[Is])...);
						};
						// clang-format on
						if constexpr (ignore_return)
						{
							unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args);
							return any{};
						}
						else
							return forward_any(unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args));
					}
					else if constexpr (is_member_like)
					{
						// clang-format off
						constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, any_ref i, std::span<any> &a) -> decltype(auto)
						{
							assert_mutable_any(i, type_name<std::remove_cvref_t<T>>());
							auto *ptr = i.template as_ptr<T>();
							return F(ptr, unwrap_any_args(type_seq_selector<Is, type_seq_t<Args...>>, a[Is])...);
						};
						// clang-format on
						if constexpr (ignore_return)
						{
							unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args);
							return {};
						}
						else
							return forward_any(unwrap(std::make_index_sequence<sizeof...(Args)>(), {instance}, args));
					}
					else
					{
						// clang-format off
						constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, std::span<any> &a) -> decltype(auto)
						{
							return F(unwrap_any_args(type_seq_selector<Is, type_seq_t<Args...>>, a[Is])...);
						};
						// clang-format on
						if constexpr (ignore_return)
						{
							unwrap(std::make_index_sequence<sizeof...(Args)>(), args);
							return any{};
						}
						else
							return forward_any(unwrap(std::make_index_sequence<sizeof...(Args)>(), args));
					}
				},
			};
			if (!node.next) [[likely]]
			{
				node.name = func_name;
				funcs.insert(node);
			}
		}
		template<typename T, typename P>
		void type_data::add_parent() noexcept
		{
			constinit static parent_node node{
				type_handle(type_selector<P>),
				+[](any_ref child) -> any_ref
				{
					if (child.type() != type_info::get<T>()) [[unlikely]]
						return any{};
					/* Need to forward const-ness. */
					return child.is_const() ? forward_any(*static_cast<const P *>(child.template as_cptr<T>())) :
											  forward_any(*static_cast<P *>(child.template as_ptr<T>()));
				},
			};

			if (!node.next) [[likely]]
				parents.insert(node);
		}
		template<typename T, typename U>
		void type_data::add_conv() noexcept
		{
			constinit static conv_node node{
				type_handle(type_selector<U>),
				+[](any instance) -> any
				{
					// clang-format off
					constexpr bool good_cast = requires(T &v) { static_cast<U>(v); };
					constexpr bool good_const_cast = requires(const T &v) { static_cast<U>(v); };
					// clang-format on

					if (instance.type() == type_info::get<T>()) [[likely]]
					{
						if (!instance.is_const() && good_cast)
							return forward_any(static_cast<U>(*instance.template as_ptr<T>()));
						else if (good_const_cast)
							return forward_any(static_cast<U>(*instance.template as_cptr<T>()));
					}
					return {};
				},
			};

			if (!node.next) [[likely]]
				convs.insert(node);
		}
		template<typename T, typename A, typename... As>
		void type_data::add_attrib(type_seq_t<A, As...>, auto &&initializer)
		{
			constinit static attrib_instance<T, type_seq_t<A, As...>> node;
			if (!node.next) [[likely]]
			{
				/* Need to post-initialize the `any` since arguments are passed at runtime. */
				node.init(initializer);
				attribs.insert(node);
			}
		}
		template<typename T, auto V, typename A, typename... As>
		void type_data::add_attrib(type_seq_t<A, As...>)
		{
			constinit static attrib_instance<T, type_seq_t<A, As...>, V> node;
			if (!node.next) [[likely]]
				attribs.insert(node);
		}

		template<typename T>
		constexpr type_data::flags_t make_type_flags() noexcept
		{
			constexpr auto result = (std::is_empty_v<T> ? type_data::IS_EMPTY : type_data::NO_FLAGS) |
									(std::is_bounded_array_v<T> ? type_data::HAS_EXTENT : type_data::NO_FLAGS) |
									(std::is_array_v<T> ? type_data::IS_ARRAY : type_data::NO_FLAGS) |
									(std::ranges::range<T> ? type_data::IS_RANGE : type_data::NO_FLAGS) |
									(std::is_pointer_v<T> ? type_data::IS_POINTER : type_data::NO_FLAGS) |
									(pointer_like<T> ? type_data::IS_POINTER_LIKE : type_data::NO_FLAGS);
			return static_cast<type_data::flags_t>(result);
		}
		template<typename T>
		constexpr auto select_value_type() noexcept
		{
			if constexpr (std::ranges::range<T>)
				return type_selector<std::remove_cvref_t<std::ranges::range_value_t<T>>>;
			else if constexpr (std::is_pointer_v<T>)
				return type_selector<std::remove_cvref_t<std::remove_pointer_t<T>>>;
			else if constexpr (pointer_like<T>)
				return type_selector<std::remove_cvref_t<typename T::value_type>>;
			else
				return type_selector<std::remove_cvref_t<T>>;
		}
		template<typename T>
		constexpr type_data::node_list<type_data::ctor_node> make_type_ctor_list() noexcept
		{
			if constexpr (std::is_default_constructible_v<T>)
				return {&type_data::ctor_instance<T>::value};
			else
				return {};
		}
		template<typename T>
		constexpr type_data make_type_data() noexcept
		{
			return type_data{
				.name = type_name<T>(),
				.extent = std::is_bounded_array_v<T> ? std::extent_v<T> : 0,
				.value_type = type_handle{select_value_type<T>()},
				.flags = make_type_flags<T>(),
				.constructors = make_type_ctor_list<T>(),
			};
		}
	}	 // namespace detail

	class signature_info
	{
		friend class constructor_info;
		friend class function_info;

		class arg_iterator
		{
		public:
			typedef type_info value_type;
			typedef const type_info *pointer;
			typedef const type_info *const_pointer;
			typedef type_info reference;
			typedef type_info const_reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::size_t size_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			friend class signature_info;

			constexpr explicit arg_iterator(const detail::type_handle *ptr) noexcept
				: ptr(ptr), ref_helper(ptr && ptr->get ? ptr->get() : nullptr)
			{
			}

		public:
			constexpr arg_iterator() noexcept = default;

			constexpr arg_iterator &operator++() noexcept { return operator+=(1); }
			constexpr arg_iterator &operator+=(difference_type n) noexcept
			{
				ref_helper = type_info{*(ptr += n)};
				return *this;
			}
			constexpr arg_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr arg_iterator &operator--() noexcept { return operator-=(1); }
			constexpr arg_iterator &operator-=(difference_type n) noexcept { return operator+=(-n); }
			constexpr arg_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}

			[[nodiscard]] constexpr arg_iterator operator+(difference_type n) const noexcept
			{
				return arg_iterator{ptr + n};
			}
			[[nodiscard]] constexpr arg_iterator operator-(difference_type n) const noexcept
			{
				return arg_iterator{ptr - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const arg_iterator &other) const noexcept
			{
				return ptr - other.ptr;
			}
			[[nodiscard]] friend constexpr arg_iterator operator+(difference_type n, const arg_iterator &iter) noexcept
			{
				return iter + n;
			}

			[[nodiscard]] constexpr pointer operator->() const noexcept { return &ref_helper; }
			[[nodiscard]] constexpr reference operator*() const noexcept { return ref_helper; }
			[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept
			{
				return *arg_iterator{ptr + i};
			}

			[[nodiscard]] constexpr auto operator<=>(const arg_iterator &other) const noexcept
			{
				return ptr <=> other.ptr;
			}
			[[nodiscard]] constexpr bool operator==(const arg_iterator &other) const noexcept
			{
				return ptr == other.ptr;
			}

		private:
			const detail::type_handle *ptr = nullptr;
			type_info ref_helper;
		};

		using arg_view = detail::type_data_view<arg_iterator>;

		constexpr signature_info(type_info ret_t, std::span<detail::type_handle> arg_ts) noexcept
			: m_ret(ret_t), m_args(arg_ts)
		{
		}

	public:
		signature_info() = delete;

		/** Returns type info of the return type of this signature.
		 * @note Returns invalid type info if the signature is a constructor signature. */
		[[nodiscard]] constexpr type_info ret() const noexcept { return m_ret; }
		/** Returns a view of argument types of this signature. */
		[[nodiscard]] constexpr arg_view args() const noexcept
		{
			return arg_view{arg_iterator{std::to_address(m_args.begin())}, arg_iterator{std::to_address(m_args.end())}};
		}

		/** Checks if the signature is invocable with a set of arguments. */
		[[nodiscard]] constexpr bool invocable_with(std::span<type_info> types) const noexcept
		{
			if (const auto argc = types.size(); argc != m_args.size())
				return false;
			else
			{
				for (std::size_t i = 0; i < argc; ++i)
					if (type_info{m_args[i]} != types[i]) [[unlikely]]
						return false;
				return true;
			}
		}
		/** @copydoc invocable_with */
		[[nodiscard]] constexpr bool invocable_with(std::span<any> argv) const noexcept
		{
			if (const auto argc = argv.size(); argc != m_args.size())
				return false;
			else
			{
				for (std::size_t i = 0; i < argc; ++i)
					if (type_info{m_args[i]} != argv[i].type()) [[unlikely]]
						return false;
				return true;
			}
		}

	private:
		[[nodiscard]] SEK_API bool assert_args(std::span<any> values) const;

		type_info m_ret; /* Invalid if constructor signature. */
		std::span<detail::type_handle> m_args;
	};

	class constructor_info
	{
		friend class detail::type_node_iterator<constructor_info>;
		friend class type_info;

		using node_t = detail::type_data::ctor_node;

		constexpr constructor_info() noexcept = default;
		constexpr explicit constructor_info(const node_t *node) noexcept : m_node(node) {}

	public:
		constexpr constructor_info(const constructor_info &) noexcept = default;
		constexpr constructor_info &operator=(const constructor_info &) noexcept = default;
		constexpr constructor_info(constructor_info &&) noexcept = default;
		constexpr constructor_info &operator=(constructor_info &&) noexcept = default;

		/** Returns signature info of the constructor.
		 * @note Return type of constructor signature is always invalid. */
		[[nodiscard]] constexpr signature_info signature() const noexcept { return {{}, m_node->arg_types}; }

		/** Invokes the underlying constructor, producing an instance of `any`.
		 * @param args Arguments passed to the constructor.
		 * @return Instance created from the constructor.
		 * @throw bad_any_type If the constructor cannot be invoked with the passed arguments.
		 * @throw bad_any_const If const-ness of the passed arguments is invalid (expected non-const but got const). */
		[[nodiscard]] any invoke(std::span<any> args) const
		{
			if (signature().assert_args(args)) [[likely]]
				return m_node->invoke(args);
			return {};
		}

		[[nodiscard]] constexpr bool operator==(const constructor_info &) const noexcept = default;

	private:
		const node_t *m_node = nullptr;
	};

	constexpr detail::type_data_view<type_info::constructor_iterator> type_info::constructors() const noexcept
	{
		return {constructor_iterator{constructor_info{m_data->constructors.front}}, {}};
	}
	constexpr bool type_info::constructable_with(std::span<type_info> args) const noexcept
	{
		return std::ranges::any_of(constructors(), [&args](auto c) { return c.signature().invocable_with(args); });
	}
	constexpr bool type_info::constructable_with(std::span<any> args) const noexcept
	{
		return std::ranges::any_of(constructors(), [&args](auto c) { return c.signature().invocable_with(args); });
	}
	// clang-format off
	template<typename... AnyArgs>
	any type_info::construct(AnyArgs &&...args) const requires detail::any_args<AnyArgs...>
	{
		std::array<any, sizeof...(AnyArgs)> args_array = {std::move(args)...};
		return construct({args_array});
	}
	// clang-format on

	class function_info
	{
		friend class detail::type_node_iterator<function_info>;
		friend class type_info;

		using node_t = detail::type_data::func_node;

		constexpr function_info() noexcept = default;
		constexpr explicit function_info(const node_t *node) noexcept : m_node(node) {}

	public:
		constexpr function_info(const function_info &) noexcept = default;
		constexpr function_info &operator=(const function_info &) noexcept = default;
		constexpr function_info(function_info &&) noexcept = default;
		constexpr function_info &operator=(function_info &&) noexcept = default;

		/** Returns reflected name of the function. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_node->name; }
		/** Returns signature info of the function. */
		[[nodiscard]] constexpr signature_info signature() const noexcept { return {{}, m_node->arg_types}; }

		/** Invokes the underlying function.
		 * @param instance Instance of the object this function is invoked on.
		 * If `function_info` represents a static function who's first argument is not an instance pointer, instance is ignored.
		 * @param args Arguments passed to the function.
		 * @return Value returned by the function. If the underlying function's return type is void, returns an empty `any`.
		 * @throw bad_any_type If the function cannot be invoked with the passed arguments.
		 * @throw bad_any_const If const-ness of the passed arguments is invalid (expected non-const but got const).
		 * @warning Invoking a non-const function on a const object (or incorrect instance type)
		 * will result in undefined behavior. */
		any invoke(any instance, std::span<any> args) const
		{
			if (signature().assert_args(args)) [[likely]]
				return m_node->invoke(std::move(instance), args);
			return {};
		}

		[[nodiscard]] constexpr bool operator==(const function_info &) const noexcept = default;

	private:
		const node_t *m_node = nullptr;
	};

	constexpr detail::type_data_view<type_info::function_iterator> type_info::functions() const noexcept
	{
		return {function_iterator{function_info{m_data->funcs.front}}, {}};
	}
	// clang-format off
	template<typename... AnyArgs>
	any type_info::invoke(std::string_view name, any instance, AnyArgs &&...args) const requires detail::any_args<AnyArgs...>
	{
		std::array<any, sizeof...(AnyArgs)> args_array = {std::move(args)...};
		return invoke(name, std::move(instance), {args_array});
	}
	// clang-format on

	class parent_info
	{
		friend class detail::type_node_iterator<parent_info>;
		friend class type_info;

		using node_t = detail::type_data::parent_node;

		constexpr parent_info() noexcept = default;
		constexpr explicit parent_info(const node_t *node) noexcept : m_node(node) {}

	public:
		constexpr parent_info(const parent_info &) noexcept = default;
		constexpr parent_info &operator=(const parent_info &) noexcept = default;
		constexpr parent_info(parent_info &&) noexcept = default;
		constexpr parent_info &operator=(parent_info &&) noexcept = default;

		/** Returns type info of the parent type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{m_node->type}; }

		/** Casts an `any` instance containing a reference to an oobject of child type to
		 * an `any` reference of parent type (preserving const-ness).
		 * @note Passed `any` instance must be a reference. Passing a non-reference `any` will result in
		 * undefined behavior (likely a crash). */
		[[nodiscard]] any_ref cast(any_ref child) const { return m_node->cast(std::move(child)); }

		[[nodiscard]] constexpr bool operator==(const parent_info &) const noexcept = default;

	private:
		const node_t *m_node = nullptr;
	};

	constexpr detail::type_data_view<type_info::parent_iterator> type_info::parents() const noexcept
	{
		return {parent_iterator{parent_info{m_data->parents.front}}, {}};
	}

	class conversion_info
	{
		friend class detail::type_node_iterator<conversion_info>;
		friend class type_info;

		using node_t = detail::type_data::conv_node;

		constexpr conversion_info() noexcept = default;
		constexpr explicit conversion_info(const node_t *node) noexcept : m_node(node) {}

	public:
		constexpr conversion_info(const conversion_info &) noexcept = default;
		constexpr conversion_info &operator=(const conversion_info &) noexcept = default;
		constexpr conversion_info(conversion_info &&) noexcept = default;
		constexpr conversion_info &operator=(conversion_info &&) noexcept = default;

		/** Returns type info of the converted-to type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{m_node->type}; }

		/** Converts an `any` instance containing an object (or reference to one) of source type to
		 * an `any` instance of converted-to type (as if via `static_cast`).
		 * If such cast is not possible, returns empty `any`. */
		[[nodiscard]] any convert(any child) const { return m_node->convert(std::move(child)); }

		[[nodiscard]] constexpr bool operator==(const conversion_info &) const noexcept = default;

	private:
		const node_t *m_node = nullptr;
	};

	constexpr detail::type_data_view<type_info::conversion_iterator> type_info::conversions() const noexcept
	{
		return {conversion_iterator{conversion_info{m_data->convs.front}}, {}};
	}

	class attribute_info
	{
		friend class detail::type_node_iterator<attribute_info>;
		friend class type_info;

		using node_t = detail::type_data::attrib_node;

		constexpr attribute_info() noexcept = default;
		constexpr explicit attribute_info(const node_t *node) noexcept : m_node(node) {}

	public:
		constexpr attribute_info(const attribute_info &) noexcept = default;
		constexpr attribute_info &operator=(const attribute_info &) noexcept = default;
		constexpr attribute_info(attribute_info &&) noexcept = default;
		constexpr attribute_info &operator=(attribute_info &&) noexcept = default;

		/** Returns type info of the attribute. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{m_node->type}; }

		/** Returns `any` reference to the attribute data. */
		[[nodiscard]] any_ref value() const noexcept { return {m_node->get_any(m_node)}; }
		/** Returns raw pointer to attribute's data. */
		[[nodiscard]] const void *data() const noexcept { return value().cdata(); }

		[[nodiscard]] bool operator==(const attribute_info &other) const noexcept
		{
			return m_node == other.m_node || (m_node && other.m_node && value() == other.value());
		}

	private:
		const node_t *m_node = nullptr;
	};

	constexpr detail::type_data_view<type_info::attribute_iterator> type_info::attributes() const noexcept
	{
		return {attribute_iterator{attribute_info{m_data->attribs.front}}, {}};
	}
	template<typename T>
	any type_info::get_attribute() const noexcept
	{
		return get_attribute(type_name<std::remove_cvref_t<T>>());
	}

	template<typename T>
	T *any::try_cast() noexcept
	{
		if constexpr (std::is_const_v<T>)
			return static_cast<const any *>(this)->template try_cast<T>();
		else if (const auto t_info = type_info::get<T>(); m_info == t_info)
			return static_cast<T *>(data());
		else if constexpr (std::is_class_v<T> && !std::is_union_v<T>) /* Ignore non-inheritable types. */
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = m_info.parents();
			auto iter = std::find_if(parents.begin(), parents.end(), [t_info](auto p) { return p.type() == t_info; });
			if (iter != parents.end()) [[likely]]
			{
				auto p_cast = iter->cast(*this);
				if (!p_cast.is_const()) [[likely]]
					return static_cast<T *>(p_cast.data());
			}

			/* No immediate parent found, search up the inheritance hierarchy. */
			for (auto p : parents)
			{
				auto p_ptr = p.cast(*this).template try_cast<T>();
				if (p_ptr != nullptr) [[likely]]
					return p_ptr;
			}
		}
		return nullptr;
	}
	template<typename T>
	std::add_const_t<T> *any::try_cast() const noexcept
	{
		using U = std::add_const_t<T>;
		if (const auto t_info = type_info::get<T>(); m_info == t_info)
			return static_cast<U *>(data());
		else if constexpr (std::is_class_v<T> && !std::is_union_v<T>) /* Ignore non-inheritable types. */
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = m_info.parents();
			auto iter = std::find_if(parents.begin(), parents.end(), [t_info](auto p) { return p.type() == t_info; });
			if (iter != parents.end()) [[likely]]
				return static_cast<U *>(iter->cast(*this).cdata());

			/* No immediate parent found, search up the inheritance hierarchy. */
			for (auto p : parents)
			{
				auto p_ptr = p.cast(*this).template try_cast<U>();
				if (p_ptr != nullptr) [[likely]]
					return p_ptr;
			}
		}
		return nullptr;
	}
	// clang-format off
	template<typename... AnyArgs>
	any any::invoke(std::string_view name, AnyArgs &&...args) requires detail::any_args<AnyArgs...>
	{
		return type().invoke(name, ref(), std::forward<AnyArgs>(args)...);
	}
	template<typename... AnyArgs>
	any any::invoke(std::string_view name, AnyArgs &&...args) const requires detail::any_args<AnyArgs...>
	{
		return type().invoke(name, ref(), std::forward<AnyArgs>(args)...);
	}
	// clang-format on

	template<typename T>
	T *any_ref::try_cast() noexcept
	{
		return value().template try_cast<T>();
	}
	template<typename T>
	std::add_const_t<T> *any_ref::try_cast() const noexcept
	{
		return value().template try_cast<T>();
	}
	template<typename T>
	T any_ref::cast()
	{
		return value().template cast<T>();
	}
	template<typename T>
	T any_ref::cast() const
	{
		return value().template cast<T>();
	}
	// clang-format off
	template<typename... AnyArgs>
	any any_ref::invoke(std::string_view name, AnyArgs &&...args) requires detail::any_args<AnyArgs...>
	{
		return type().invoke(name, *this, std::forward<AnyArgs>(args)...);
	}
	template<typename... AnyArgs>
	any any_ref::invoke(std::string_view name, AnyArgs &&...args) const requires detail::any_args<AnyArgs...>
	{
		return type().invoke(name, *this, std::forward<AnyArgs>(args)...);
	}
	// clang-format on

	namespace literals
	{
		/** Retrieves a reflected type from the runtime database. */
		[[nodiscard]] inline type_info operator""_type(const char *str, std::size_t n)
		{
			return type_info::get({str, n});
		}
	}	 // namespace literals
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::shared_guard<sek::engine::type_database>>;

/** Macro used to declare an instance of type info for type `T` as extern.
 *
 * @note Type must be exported via `SEK_EXPORT_TYPE`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE(my_type)
 * @endcode*/
#define SEK_EXTERN_TYPE(T)                                                                                             \
	extern template SEK_API_IMPORT sek::engine::type_info::data_t *sek::engine::type_info::get_data<T>();

/** Macro used to export instance of type info for type `T`.
 *
 * @note Type must be declared as `extern` via `SEK_EXTERN_TYPE`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE(my_type)
 * @endcode */
#define SEK_EXPORT_TYPE(T)                                                                                             \
	template SEK_API_EXPORT sek::engine::type_info::data_t *sek::engine::type_info::get_data<T>();
