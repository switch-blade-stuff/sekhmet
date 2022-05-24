//
// Created by switchblade on 2022-01-23.
//

#pragma once

#include <new>
#include <span>
#include <utility>

#include "aligned_storage.hpp"
#include "assert.hpp"
#include "type_name.hpp"

namespace sek
{
	class any;
	class any_ref;
	class type_info;

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
				std::span<type_handle> arg_types;
				any (*invoke)(std::span<any>);
			};
			struct attrib_node;
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
			template<typename T, typename P>
			void add_parent() noexcept;
			template<typename T, typename U>
			void add_conv() noexcept;
			template<typename T, typename A, typename... As>
			void add_attrib(type_seq_t<A, As...>, auto &&);

			const std::string_view name;
			const std::size_t extent;
			const type_handle value_type; /* Underlying value type of either a pointer or a range. */
			const flags_t flags;

			void (*destructor)(any) = nullptr; /* Placement destructor. */
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
			constexpr explicit type_node_iterator(V node_value) noexcept : node_value(node_value) {}
			constexpr type_node_iterator() noexcept = default;

			constexpr type_node_iterator &operator++() noexcept
			{
				node_value.node = node()->next;
				return *this;
			}
			constexpr type_node_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}

			[[nodiscard]] constexpr pointer operator->() const noexcept { return &node_value; }
			[[nodiscard]] constexpr reference operator*() const noexcept { return node_value; }

			[[nodiscard]] constexpr bool operator==(const type_node_iterator &other) const noexcept
			{
				return node_value == other.node_value;
			}

		private:
			[[nodiscard]] constexpr auto *node() const noexcept { return node_value.node; }

			value_type node_value;
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
			constexpr type_data_view(iterator first, iterator last) noexcept : first(first), last(last) {}

			[[nodiscard]] constexpr iterator begin() const noexcept { return first; }
			[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr iterator end() const noexcept { return last; }
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
				return std::equal(first, last, other.first, other.last);
			}

		private:
			Iter first;
			Iter last;
		};
	}	 // namespace detail

	/** Exception thrown when the type of `any` is not as expected. */
	class bad_any_type : public std::runtime_error
	{
	public:
		bad_any_type() : std::runtime_error("Invalid type of `any` object") {}
		explicit bad_any_type(const std::string &msg) : std::runtime_error(msg) {}
		explicit bad_any_type(const char *msg) : std::runtime_error(msg) {}
		~bad_any_type() override = default;
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

	/** @brief Structure used to reference reflected information about a type. */
	class type_info
	{
		friend struct detail::type_handle;
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

		SEK_API static data_t &register_type(handle_t handle) noexcept;

	public:
		template<typename T, typename... Attr>
		class type_factory
		{
			friend class type_info;

			constexpr explicit type_factory(data_t &data) noexcept : data(data) {}

			// clang-format off
			template<typename U>
			constexpr static bool good_cast = requires (T v) { static_cast<U>(v); };
			template<typename U>
			constexpr static bool unqualified = std::same_as<std::remove_cvref_t<U>, U>;
			template<typename U>
			constexpr static bool different = !std::same_as<std::remove_cvref_t<U>, T>;
			// clang-format on

		public:
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
				data.template add_ctor<T, Args...>();
				return *this;
			}
			/** Adds `P` to the list of parents of `T`.
			 * @tparam P Parent type of `T`. */
			template<typename P>
			type_factory &parent() requires std::derived_from<T, P> && unqualified<P> && different<P>
			{
				data.template add_parent<T, P>();
				return *this;
			}
			/** Adds `U` to `T`'s list of conversions.
			 * @tparam U Type that `T` can be `static_cast` to. */
			template<typename U>
			type_factory &convertible() requires good_cast<U> && unqualified<U> && different<U>
			{
				data.template add_conv<T, U>();
				return *this;
			}
			/** Adds an attribute to `T`'s list of attributes.
			 * @tparam A Type of the attribute. */
			template<typename A, typename... Args>
			type_factory<T, A, Attr...> attribute(Args &&...args)
			{
				data.template add_attrib<T>(type_seq<A, Attr...>, [&](){ return make_any<A>(std::forward<Args>(args)...); });
				return type_factory<T, A, Attr...>{data};
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
				data.template add_attrib<T>(type_seq<A, Attr...>, [](){ return forward_any(auto_constant<Value>::value); });
				return type_factory<T, A, Attr...>{data};
			}
			// clang-format on

		private:
			data_t &data;
		};

	public:
		/** Returns type info for type `T`.
		 * @note Removes any const & volatile qualifiers and decays references. */
		template<typename T>
		[[nodiscard]] constexpr static type_info get() noexcept
		{
			return type_info{get_handle<T>()};
		}

		/** Reflects type `T`, making it available for runtime lookup by-name.
		 * @return Type factory for type `T`, which can be used to specify additional information about the type.
		 * @note Removes any const & volatile qualifiers and decays references. */
		template<typename T>
		constexpr static type_factory<T> reflect() noexcept
		{
			return type_factory<T>{register_type(get_handle<T>())};
		}
		/** Searches for a reflected type in internal database.
		 * @return Type info of the type, or an invalid type info if such type is not found. */
		SEK_API static type_info get(std::string_view name) noexcept;
		/** Resets a reflected type, removing it from internal database.
		 * @note The type will no longer be available for runtime lookup. */
		SEK_API static void reset(std::string_view name) noexcept;
		/** @copydoc reset
		 * @note Removes any const & volatile qualifiers and decays references. */
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

		using parent_iterator = detail::type_node_iterator<parent_info>;
		using conversion_iterator = detail::type_node_iterator<conversion_info>;
		using constructor_iterator = detail::type_node_iterator<constructor_info>;
		using attribute_iterator = detail::type_node_iterator<attribute_info>;

		constexpr explicit type_info(const data_t *data) noexcept : data(data) {}
		constexpr explicit type_info(handle_t handle) noexcept : data(handle.get()) {}

	public:
		/** Initializes an invalid type info (type info with no underlying type). */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a reflected type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return data != nullptr; }
		/** @copydoc valid */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns the name of the underlying type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return data->name; }

		/** Checks if the underlying type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return data->is_empty(); }
		/** Checks if the underlying type has an extent (is a bounded array). */
		[[nodiscard]] constexpr bool has_extent() const noexcept { return data->has_extent(); }
		/** Checks if the underlying type is an array. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return data->is_array(); }
		/** Checks if the underlying type is a range. */
		[[nodiscard]] constexpr bool is_range() const noexcept { return data->is_range(); }
		/** Checks if the underlying type is a pointer. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return data->is_pointer(); }
		/** Checks if the underlying type is a pointer-like object. */
		[[nodiscard]] constexpr bool is_pointer_like() const noexcept { return data->is_pointer_like(); }

		/** Returns the extent of the underlying type.
		 * @note If the type is not a bounded array, extent is 0. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept { return data->extent; }
		/** Returns value type oof the underlying range, pointer or pointer-like type.
		 * @note If the type is not a range, pointer or pointer-like, returns identity. */
		[[nodiscard]] constexpr type_info value_type() const noexcept { return type_info{data->value_type}; }

		/** Returns a range of constructors of this type. */
		[[nodiscard]] constexpr detail::type_data_view<constructor_iterator> constructors() const noexcept;
		/** Checks if the type is constructable with the specified set of arguments. */
		[[nodiscard]] constexpr bool constructable_with(std::span<type_info> args) const noexcept;
		/** @copydoc constructable_with */
		[[nodiscard]] constexpr bool constructable_with(std::span<any> args) const noexcept;
		/** Constructs the underlying type with the passed arguments & returns an `any` managing the constructed object.
		 * @param args Arguments passed to the constructor.
		 * @return `any` managing the constructed object.
		 * @throw bad_any_type If no constructor accepting `args` was found. */
		[[nodiscard]] any construct(std::span<any> args = {}) const;
		// clang-format off
		/** @copydoc construct */
		template<typename... AnyArgs>
		[[nodiscard]] any construct(AnyArgs &&...args) const requires std::conjunction_v<std::is_same<std::remove_cvref_t<AnyArgs>, any>...>;
		// clang-format on

		/** Returns a range of parents of this type. */
		[[nodiscard]] constexpr detail::type_data_view<parent_iterator> parents() const noexcept;
		/** Checks if the underlying type inherits the specified type. */
		[[nodiscard]] constexpr bool inherits(type_info info) const noexcept
		{
			const auto pred = [info](auto &n) { return type_info{n.type} == info || type_info{n.type}.inherits(info); };
			return std::ranges::any_of(data->parents, pred);
		}
		/** Checks if the underlying type inherits a type with the specified name. */
		[[nodiscard]] constexpr bool inherits(std::string_view name) const noexcept
		{
			const auto pred = [name](auto &n) { return n.type->name == name || type_info{n.type}.inherits(name); };
			return std::ranges::any_of(data->parents, pred);
		}
		/** Checks if the underlying type inherits 'T'. */
		template<typename T>
		[[nodiscard]] constexpr bool inherits() const noexcept
		{
			return inherits(type_name<T>());
		}

		/** Returns a range of attributes of this type. */
		[[nodiscard]] constexpr detail::type_data_view<attribute_iterator> attributes() const noexcept;

		/** Returns a range of conversions of this type. */
		[[nodiscard]] constexpr detail::type_data_view<conversion_iterator> conversions() const noexcept;
		/** Checks if the underlying type is convertible to the specified type via `static_cast`. */
		[[nodiscard]] constexpr bool convertible_to(type_info info) const noexcept
		{
			const auto pred = [info](auto &n) { return type_info{n.type} == info; };
			return std::ranges::any_of(data->convs, pred);
		}
		/** Checks if the underlying type is convertible to a type with the specified name via `static_cast`. */
		[[nodiscard]] constexpr bool convertible_to(std::string_view name) const noexcept
		{
			const auto pred = [name](auto &n) { return n.type->name == name; };
			return std::ranges::any_of(data->convs, pred);
		}
		/** Checks if the underlying type is convertible to 'T' via `static_cast`. */
		template<typename T>
		[[nodiscard]] constexpr bool convertible_to() const noexcept
		{
			return convertible_to(type_name<T>());
		}

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			return data == other.data || (data && other.data && name() == other.name());
		}

		constexpr void swap(type_info &other) noexcept { std::swap(data, other.data); }
		friend constexpr void swap(type_info &a, type_info &b) noexcept { a.swap(b); }

	private:
		const data_t *data = nullptr;
	};

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
			: vtable(vtable), info(info), storage(storage), flags(flags)
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
		[[nodiscard]] constexpr type_info type() const noexcept { return info; }

		/** Checks if `any` manages an object. */
		[[nodiscard]] constexpr bool empty() const noexcept { return vtable == nullptr; }
		/** Checks if `any` references an externally-stored object. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return flags & IS_REF; }
		/** Checks if the managed object is stored in-place. */
		[[nodiscard]] constexpr bool is_local() const noexcept { return flags & IS_LOCAL; }
		/** Checks if the managed object is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return flags & IS_CONST; }

		/** Resets `any` by destroying and releasing the internal object. */
		void reset()
		{
			reset_impl();
			vtable = nullptr;
			info = {};
			storage = {};
			flags = {};
		}

		/** Returns raw pointer to the managed object's data.
		 * @note If the managed object is const-qualified, returns nullptr. */
		[[nodiscard]] constexpr void *data() noexcept
		{
			return is_const() ? nullptr : is_local() ? storage.local.data() : storage.external;
		}
		/** Returns raw const pointer to the managed object's data. */
		[[nodiscard]] constexpr const void *cdata() const noexcept
		{
			return is_local() ? storage.local.data() : storage.external;
		}
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return cdata(); }

		/** Returns an `any` referencing to the managed object.
		 * @note Preserves const-ness of the managed object. */
		[[nodiscard]] any ref() noexcept
		{
			return any{vtable, info, cdata(), static_cast<flags_t>(IS_REF | (flags & IS_CONST))};
		}
		/** Returns a const `any` referencing to the managed object. */
		[[nodiscard]] any cref() const noexcept
		{
			return any{vtable, info, cdata(), static_cast<flags_t>(IS_REF | IS_CONST)};
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
				if (info == type_info::get<T>()) [[likely]]
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
			if (info == type_info::get<U>()) [[likely]]
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
		 * @throw bad_any_type If no such cast is possible. */
		template<typename T>
		[[nodiscard]] T cast()
		{
			auto ptr = try_cast<std::remove_reference_t<T>>();
			if (!ptr) [[unlikely]]
				throw bad_any_type("Invalid any cast");
			return static_cast<T>(*ptr);
		}
		/** @copydoc cast */
		template<typename T>
		[[nodiscard]] T cast() const
		{
			auto ptr = try_cast<std::remove_reference_t<T>>();
			if (!ptr) [[unlikely]]
				throw bad_any_type("Invalid any cast");
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
		[[nodiscard]] any convert(std::string_view to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] any convert(type_info to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] any convert(std::string_view to_type) const noexcept;
		/** @copydoc convert */
		[[nodiscard]] any convert(type_info to_type) const noexcept;

		/** Compares managed objects by-value.
		 * @return true if the managed objects compare equal,
		 * false if they do not or if they are of different types. */
		[[nodiscard]] bool operator==(const any &other) const
		{
			if (empty() && other.empty()) [[unlikely]]
				return true;
			else
				return info == other.info && vtable->compare(*this, other);
		}

		constexpr void swap(any &other) noexcept
		{
			std::swap(vtable, other.vtable);
			info.swap(other.info);
			storage.swap(other.storage);
			std::swap(flags, other.flags);
		}
		friend constexpr void swap(any &a, any &b) noexcept { a.swap(b); }

	private:
		void reset_impl()
		{
			/* References are not destroyed. */
			if (flags & IS_REF) [[unlikely]]
				return;
			else if (vtable != nullptr) [[likely]]
				vtable->destroy(*this);
		}
		void copy_construct(const any &from)
		{
			if (from.vtable != nullptr) [[likely]]
				from.vtable->copy_construct(*this, from);
			vtable = from.vtable;
			info = from.info;
		}
		void copy_assign(const any &from)
		{
			if (empty() && from.vtable != nullptr) [[unlikely]]
				from.vtable->copy_construct(*this, from);
			else if (from.vtable != nullptr) [[likely]]
				from.vtable->copy_assign(*this, from);
			else
				reset();
			vtable = from.vtable;
			info = from.info;
		}

		const vtable_t *vtable = nullptr;
		type_info info = {};
		storage_t storage = {};
		flags_t flags = {};
	};

	/** @brief Type-erased reference to objects. */
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
			SEK_ASSERT(data.is_ref(), "Passed `any` must be a reference");
			std::construct_at(storage.get<any>(), std::move(data));
		}
		/** Initializes `any` reference from an `any` instance.
		 * @param data `any` containing data to be referenced. */
		any_ref(any &data) noexcept { std::construct_at(storage.get<any>(), data.ref()); }
		/** @copydoc any_ref */
		any_ref(const any &data) noexcept { std::construct_at(storage.get<any>(), data.ref()); }

		constexpr any_ref(any_ref &&other) noexcept { std::construct_at(storage.get<any>(), std::move(other.value())); }
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
		[[nodiscard]] constexpr void *data() noexcept { return !is_const() ? value().storage.external : nullptr; }
		/** @copydoc any::cdata */
		[[nodiscard]] constexpr const void *cdata() const noexcept { return value().storage.external; }
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return value().storage.external; }

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
		[[nodiscard]] any convert(std::string_view to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] any convert(type_info to_type) noexcept;
		/** @copydoc convert */
		[[nodiscard]] any convert(std::string_view to_type) const noexcept;
		/** @copydoc convert */
		[[nodiscard]] any convert(type_info to_type) const noexcept;

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
		[[nodiscard]] constexpr any &value() noexcept { return *storage.get<any>(); }
		[[nodiscard]] constexpr const any &value() const noexcept { return *storage.get<any>(); }

		type_storage<any> storage;
	};

	template<typename T>
	constinit const any::vtable_t any::vtable_instance<T>::value = {
		.copy_construct = +[](any &to, const any &from) -> void
		{
			to.storage = storage_t(std::in_place_type<T>, *static_cast<const T *>(from.data()));
			to.flags = make_flags<T>();
		},
		.copy_assign = +[](any &to, const any &from) -> void
		{
			constexpr auto reset_copy = [](any &t, const any &f)
			{
				t.reset();
				t.storage = storage_t(std::in_place_type<T>, *static_cast<const T *>(f.data()));
			};

			using Tc = std::add_const_t<T>;
			if constexpr (!requires(T & a, Tc & b) { a = b; })
				reset_copy(to, from);
			else
			{
				if (to.info != from.info)
					reset_copy(to, from);
				else if constexpr (local_candidate<T>)
					*to.storage.local.template get<T>() = *static_cast<Tc *>(from.data());
				else
					*static_cast<T *>(to.storage.external) = *static_cast<Tc *>(from.data());
			}
			to.flags = make_flags<T>();
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
				std::destroy_at(instance.storage.local.template get<T>());
			else if constexpr (std::is_bounded_array_v<T>)
				delete[] static_cast<T *>(instance.storage.external);
			else
				delete static_cast<T *>(instance.storage.external);
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

	namespace detail
	{
		struct type_data::attrib_node : basic_node<attrib_node>
		{
			any data;
		};

		template<typename T>
		constexpr decltype(auto) unwrap_any_arg(type_selector_t<T>, any &a)
		{
			using U = std::remove_reference_t<T>;
			if constexpr (!std::is_reference_v<T> || std::is_const_v<U>)
				return *a.as_cptr<U>();
			else if (a.is_const()) [[unlikely]]
				throw bad_any_type("Cannot bind const `any` to non-const reference argument");
			else
				return std::forward<T>(*a.as_ptr<U>());
		}

		template<typename T, typename... Args>
		constinit type_data::ctor_instance<T, Args...> type_data::ctor_instance<T, Args...>::value = {
			{type_handle{type_selector<std::decay_t<Args>>}...},
			+[](std::span<any> args) -> any
			{
				constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, std::span<any> & as)
				{
					return make_any<T>(unwrap_any_arg(type_selector<type_seq_element_t<Is, type_seq_t<Args...>>>, as[Is])...);
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
		void type_data::add_attrib(type_seq_t<A, As...>, auto &&generator)
		{
			constinit static attrib_node node;
			if (!node.next) [[likely]]
			{
				/* Need to post-initialize the `any` since arguments are passed at runtime. */
				node.data = generator();
				attribs.insert(node);
			}
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
		constexpr type_data make_type_data() noexcept
		{
			using ct_list = type_data::node_list<type_data::ctor_node>;
			return type_data{
				.name = type_name<T>(),
				.extent = std::is_bounded_array_v<T> ? std::extent_v<T> : 0,
				.value_type = type_handle{select_value_type<T>()},
				.flags = make_type_flags<T>(),
				.constructors = std::is_default_constructible_v<T> ? ct_list{&type_data::ctor_instance<T>::value} : ct_list{},
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
			: ret_type(ret_t), arg_types(arg_ts)
		{
		}

	public:
		signature_info() = delete;

		/** Returns type info of the return type of this signature.
		 * @note Returns invalid type info if the signature is a constructor signature. */
		[[nodiscard]] constexpr type_info ret() const noexcept { return ret_type; }
		/** Returns a view of argument types of this signature. */
		[[nodiscard]] constexpr arg_view args() const noexcept
		{
			return arg_view{arg_iterator{std::to_address(arg_types.begin())}, arg_iterator{std::to_address(arg_types.end())}};
		}

		/** Checks if the signature is invocable with a set of arguments. */
		[[nodiscard]] constexpr bool invocable_with(std::span<type_info> types) const noexcept
		{
			if (const auto argc = types.size(); argc != arg_types.size())
				return false;
			else
			{
				for (std::size_t i = 0; i < argc; ++i)
					if (type_info{arg_types[i]} != types[i]) [[unlikely]]
						return false;
				return true;
			}
		}
		/** @copydoc invocable_with */
		[[nodiscard]] constexpr bool invocable_with(std::span<any> argv) const noexcept
		{
			if (const auto argc = argv.size(); argc != arg_types.size())
				return false;
			else
			{
				for (std::size_t i = 0; i < argc; ++i)
					if (type_info{arg_types[i]} != argv[i].type()) [[unlikely]]
						return false;
				return true;
			}
		}

	private:
		[[nodiscard]] std::string make_error_msg() const
		{
			const auto as = args();

			std::string result = "Invalid argument types. Expected: [";
			for (std::size_t i = 0, max = i < as.size();;)
			{
				result.append(1, '\"').append(as[i].name()).append(1, '\"');
				if (++i != max) [[likely]]
					result.append(", ");
				else
					break;
			}
			result.append("]");

			return result;
		}
		[[nodiscard]] bool assert_args(std::span<any> values) const
		{
			if (!invocable_with(values)) [[unlikely]]
				throw bad_any_type(make_error_msg());
			return true;
		}

		type_info ret_type; /* Invalid if constructor signature. */
		std::span<detail::type_handle> arg_types;
	};

	class constructor_info
	{
		friend class detail::type_node_iterator<constructor_info>;
		friend class type_info;

		using node_t = detail::type_data::ctor_node;

		constexpr constructor_info() noexcept = default;
		constexpr explicit constructor_info(const node_t *node) noexcept : node(node) {}

	public:
		constexpr constructor_info(const constructor_info &) noexcept = default;
		constexpr constructor_info &operator=(const constructor_info &) noexcept = default;
		constexpr constructor_info(constructor_info &&) noexcept = default;
		constexpr constructor_info &operator=(constructor_info &&) noexcept = default;

		/** Returns signature info of the constructor.
		 * @note Return type of constructor signature is always invalid. */
		[[nodiscard]] constexpr signature_info signature() const noexcept { return {{}, node->arg_types}; }

		/** Invokes the underlying constructor, producing an instance of `any`.
		 * @param args Arguments passed to the constructor.
		 * @throw bad_any_type If the constructor cannot be invoked with the passed arguments. */
		[[nodiscard]] any invoke(std::span<any> args) const
		{
			if (signature().assert_args(args)) [[likely]]
				return node->invoke(args);
			return {};
		}

		[[nodiscard]] constexpr bool operator==(const constructor_info &) const noexcept = default;

	private:
		const node_t *node = nullptr;
	};

	constexpr detail::type_data_view<type_info::constructor_iterator> type_info::constructors() const noexcept
	{
		return {constructor_iterator{constructor_info{data->constructors.front}}, {}};
	}
	constexpr bool type_info::constructable_with(std::span<type_info> args) const noexcept
	{
		return std::ranges::any_of(constructors(), [&args](auto c) { return c.signature().invocable_with(args); });
	}
	constexpr bool type_info::constructable_with(std::span<any> args) const noexcept
	{
		return std::ranges::any_of(constructors(), [&args](auto c) { return c.signature().invocable_with(args); });
	}

	any type_info::construct(std::span<any> args) const
	{
		const auto ctors = constructors();
		const auto ctor = std::ranges::find_if(ctors, [&args](auto c) { return c.signature().invocable_with(args); });
		if (ctor == ctors.end()) [[unlikely]]
			throw bad_any_type("No matching constructor found");
		else
			return ctor->node->invoke(args);
	}
	// clang-format off
	template<typename... AnyArgs>
	any type_info::construct(AnyArgs &&...args) const requires std::conjunction_v<std::is_same<std::remove_cvref_t<AnyArgs>, any>...>
	{
		std::array<any, sizeof...(AnyArgs)> args_array = {std::move(args)...};
		return construct({args_array});
	}
	// clang-format on

	class parent_info
	{
		friend class detail::type_node_iterator<parent_info>;
		friend class type_info;

		using node_t = detail::type_data::parent_node;

		constexpr parent_info() noexcept = default;
		constexpr explicit parent_info(const node_t *node) noexcept : node(node) {}

	public:
		constexpr parent_info(const parent_info &) noexcept = default;
		constexpr parent_info &operator=(const parent_info &) noexcept = default;
		constexpr parent_info(parent_info &&) noexcept = default;
		constexpr parent_info &operator=(parent_info &&) noexcept = default;

		/** Returns type info of the parent type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{node->type}; }

		/** Casts an `any` instance containing a reference to an oobject of child type to
		 * an `any` reference of parent type (preserving const-ness).
		 * @note Passed `any` instance must be a reference. Passing a non-reference `any` will result in
		 * undefined behavior (likely a crash). */
		[[nodiscard]] any_ref cast(any_ref child) const { return node->cast(std::move(child)); }

		[[nodiscard]] constexpr bool operator==(const parent_info &) const noexcept = default;

	private:
		const node_t *node = nullptr;
	};
	class conversion_info
	{
		friend class detail::type_node_iterator<conversion_info>;
		friend class type_info;

		using node_t = detail::type_data::conv_node;

		constexpr conversion_info() noexcept = default;
		constexpr explicit conversion_info(const node_t *node) noexcept : node(node) {}

	public:
		constexpr conversion_info(const conversion_info &) noexcept = default;
		constexpr conversion_info &operator=(const conversion_info &) noexcept = default;
		constexpr conversion_info(conversion_info &&) noexcept = default;
		constexpr conversion_info &operator=(conversion_info &&) noexcept = default;

		/** Returns type info of the converted-to type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{node->type}; }

		/** Converts an `any` instance containing an object (or reference to one) of source type to
		 * an `any` instance of converted-to type (as if via `static_cast`).
		 * If such cast is not possible, returns empty `any`. */
		[[nodiscard]] any convert(any child) const { return node->convert(std::move(child)); }

		[[nodiscard]] constexpr bool operator==(const conversion_info &) const noexcept = default;

	private:
		const node_t *node = nullptr;
	};
	class attribute_info
	{
		friend class detail::type_node_iterator<attribute_info>;
		friend class type_info;

		using node_t = detail::type_data::attrib_node;

		constexpr attribute_info() noexcept = default;
		constexpr explicit attribute_info(const node_t *node) noexcept : node(node) {}

	public:
		constexpr attribute_info(const attribute_info &) noexcept = default;
		constexpr attribute_info &operator=(const attribute_info &) noexcept = default;
		constexpr attribute_info(attribute_info &&) noexcept = default;
		constexpr attribute_info &operator=(attribute_info &&) noexcept = default;

		/** Returns type info of the attribute. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{node->data.type()}; }

		/** Returns raw pointer to attribute's data. */
		[[nodiscard]] constexpr const void *data() const noexcept { return node->data.cdata(); }
		/** Returns `any` reference to the attribute data. */
		[[nodiscard]] any_ref value() const noexcept { return {node->data.ref()}; }

		[[nodiscard]] bool operator==(const attribute_info &other) const noexcept
		{
			return node == other.node || (node && other.node && node->data == other.node->data);
		}

	private:
		const node_t *node = nullptr;
	};

	constexpr detail::type_data_view<type_info::parent_iterator> type_info::parents() const noexcept
	{
		return {parent_iterator{parent_info{data->parents.front}}, {}};
	}
	constexpr detail::type_data_view<type_info::conversion_iterator> type_info::conversions() const noexcept
	{
		return {conversion_iterator{conversion_info{data->convs.front}}, {}};
	}
	constexpr detail::type_data_view<type_info::attribute_iterator> type_info::attributes() const noexcept
	{
		return {attribute_iterator{attribute_info{data->attribs.front}}, {}};
	}

	template<typename T>
	T *any::try_cast() noexcept
	{
		if constexpr (std::is_const_v<T>)
			return static_cast<const any *>(this)->template try_cast<T>();
		else if (const auto t_info = type_info::get<T>(); info == t_info)
			return static_cast<T *>(data());
		else if constexpr (std::is_class_v<T> && !std::is_union_v<T>) /* Ignore non-inheritable types. */
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = info.parents();
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
		if (const auto t_info = type_info::get<T>(); info == t_info)
			return static_cast<U *>(data());
		else if constexpr (std::is_class_v<T> && !std::is_union_v<T>) /* Ignore non-inheritable types. */
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = info.parents();
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

	any any::convert(std::string_view n) noexcept
	{
		if (info.name() == n)
			return ref();
		else
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = info.parents();
			auto p_iter = std::find_if(parents.begin(), parents.end(), [n](auto p) { return p.type().name() == n; });
			if (p_iter != parents.end()) [[likely]]
				return p_iter->cast(*this);

			/* Attempt to cast to an explicit conversion. */
			const auto convs = info.conversions();
			auto conv_iter = std::find_if(convs.begin(), convs.end(), [n](auto c) { return c.type().name() == n; });
			if (conv_iter != convs.end()) [[likely]]
				return conv_iter->convert(ref());

			/* Search up the inheritance hierarchy. */
			for (auto p : parents)
			{
				auto p_result = p.cast(*this).convert(n);
				if (!p_result.empty()) [[likely]]
					return p_result;
			}

			return {};
		}
	}
	any any::convert(type_info to_type) noexcept { return convert(to_type.name()); }
	any any::convert(std::string_view n) const noexcept
	{
		if (info.name() == n)
			return ref();
		else
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = info.parents();
			auto p_iter = std::find_if(parents.begin(), parents.end(), [n](auto p) { return p.type().name() == n; });
			if (p_iter != parents.end()) [[likely]]
				return p_iter->cast(*this);

			/* Attempt to cast to an explicit conversion. */
			const auto convs = info.conversions();
			auto conv_iter = std::find_if(convs.begin(), convs.end(), [n](auto c) { return c.type().name() == n; });
			if (conv_iter != convs.end()) [[likely]]
				return conv_iter->convert(ref());

			/* Search up the inheritance hierarchy. */
			for (auto p : parents)
			{
				const auto p_cast = p.cast(*this);
				auto p_result = p_cast.convert(n);
				if (!p_result.empty()) [[likely]]
					return p_result;
			}

			return {};
		}
	}
	any any::convert(type_info to_type) const noexcept { return convert(to_type.name()); }

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
	any any_ref::convert(std::string_view n) noexcept { return value().convert(n); }
	any any_ref::convert(type_info to_type) noexcept { return value().convert(to_type); }
	any any_ref::convert(std::string_view n) const noexcept { return value().convert(n); }
	any any_ref::convert(type_info to_type) const noexcept { return value().convert(to_type); }

	namespace literals
	{
		/** Retrieves a reflected type from the runtime database. */
		[[nodiscard]] type_info operator""_type(const char *str, std::size_t n) { return type_info::get({str, n}); }
	}	 // namespace literals
}	 // namespace sek

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
#define SEK_EXTERN_TYPE(T) extern template SEK_API_IMPORT sek::type_info::data_t *sek::type_info::get_data<T>();

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
#define SEK_EXPORT_TYPE(T) template SEK_API_EXPORT sek::type_info::data_t *sek::type_info::get_data<T>();
