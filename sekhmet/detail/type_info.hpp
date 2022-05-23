//
// Created by switchblade on 2022-01-23.
//

#pragma once

#include <new>
#include <span>

#include "aligned_storage.hpp"
#include "assert.hpp"
#include "static_string.hpp"

namespace sek
{
	namespace detail
	{
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name(basic_static_string<char, N> result) noexcept
		{
			if constexpr (I == Last)
			{
				result[J] = '\0';
				return result;
			}
			else
			{
				result[J] = static_cast<typename decltype(result)::value_type>(Src[I]);
				return format_type_name<Src, J + 1, I + 1, Last>(result);
			}
		}
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name() noexcept
		{
			return format_type_name<Src, J, I, Last, N>({});
		}
		template<basic_static_string Name>
		consteval auto format_type_name() noexcept
		{
#if defined(__clang__) || defined(__GNUC__)
			constexpr auto offset_start = Name.find_first('=') + 2;
			constexpr auto offset_end = Name.find_last(']');
			constexpr auto trimmed_length = offset_end - offset_start + 1;
#elif defined(_MSC_VER)
			constexpr auto offset_start = Name.find_first('<') + 1;
			constexpr auto offset_end = Name.find_last('>');
			constexpr auto trimmed_length = offset_end - offset_start + 1;
#else
#error "Implement type name generation for this compiler"
#endif
			return format_type_name<Name, 0, offset_start, offset_end, trimmed_length>();
		}

		template<typename T>
		[[nodiscard]] constexpr std::basic_string_view<char> generate_type_name() noexcept
		{
			constexpr auto &value = auto_constant<format_type_name<SEK_PRETTY_FUNC>()>::value;
			return std::basic_string_view<char>{value.begin(), value.end()};
		}
	}	 // namespace detail

	/** Returns name of the specified type.
	 * @warning Consistency of generated type names across different compilers is not guaranteed.
	 * To generate consistent type names, overload this function for the desired type. */
	template<typename T>
	[[nodiscard]] constexpr std::string_view type_name() noexcept
	{
		return detail::generate_type_name<T>();
	}

	class any;
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
			template<typename T>
			constexpr explicit type_handle(type_selector_t<T>) noexcept;

			[[nodiscard]] constexpr type_data *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr type_data &operator*() const noexcept { return *get(); }

			type_data *(*get)() noexcept;
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

				constexpr ctor_node(std::span<const type_handle> args,
									void (*invoke_at)(any, std::span<any>),
									any (*invoke)(std::span<any>)) noexcept
					: arg_types(args), invoke_at(invoke_at), invoke(invoke)
				{
				}

				std::span<const type_handle> arg_types;
				void (*invoke_at)(any, std::span<any>); /* Placement constructor. */
				any (*invoke)(std::span<any>);			/* Allocating constructor. */
			};
			struct parent_node : basic_node<parent_node>
			{
				constexpr parent_node(type_handle type, any (*cast)(any)) noexcept : type(type), cast(cast) {}

				type_handle type;
				any (*cast)(any);
			};
			struct attrib_node;
			struct func_node : basic_node<func_node>
			{
				std::span<const type_handle> arg_types;
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
			template<typename T, typename A, typename... As>
			void add_attrib(type_seq_t<A, As...>, auto &&);

			const std::string_view name;
			const std::size_t extent;
			const type_handle value_type; /* Underlying value type of either a pointer or a range. */
			const flags_t flags;

			void (*dtor)(any) = nullptr; /* Placement destructor. */
			node_list<ctor_node> ctors = {};

			node_list<parent_node> parents = {};
			node_list<attrib_node> attribs = {};
			node_list<func_node> funcs = {};
		};

		template<typename T>
		constexpr type_data make_type_data() noexcept;
	}	 // namespace detail

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

		template<typename V>
		class data_node_iterator
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

		private:
			constexpr explicit data_node_iterator(V node_value) noexcept : node_value(node_value) {}

		public:
			constexpr data_node_iterator() noexcept = default;

			constexpr data_node_iterator &operator++() noexcept
			{
				node_value.node = node()->next;
				return *this;
			}
			constexpr data_node_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}

			[[nodiscard]] constexpr pointer operator->() const noexcept { return &node_value; }
			[[nodiscard]] constexpr reference operator*() const noexcept { return node_value; }

			[[nodiscard]] constexpr bool operator==(const data_node_iterator &) const noexcept = default;

		private:
			[[nodiscard]] constexpr auto *node() const noexcept { return node_value.node; }

			value_type node_value;
		};

		/* Custom view, as CLang has issues with `std::ranges::subrange` at this time. */
		template<typename Iter>
		class data_node_view
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
			constexpr data_node_view() noexcept = default;
			constexpr data_node_view(iterator first, iterator last) noexcept : first(first), last(last) {}

			[[nodiscard]] constexpr iterator begin() const noexcept { return first; }
			[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr iterator end() const noexcept { return last; }
			[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

			[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
			[[nodiscard]] constexpr size_type size() const noexcept { return std::distance(begin(), end()); }

			[[nodiscard]] constexpr bool operator==(const data_node_view &) const noexcept = default;

		private:
			Iter first;
			Iter last;
		};

	public:
		template<typename T, typename... Attr>
		class type_factory
		{
			friend class type_info;

			constexpr explicit type_factory(data_t &data) noexcept : data(data) {}

		public:
			constexpr type_factory(const type_factory &) noexcept = default;
			constexpr type_factory &operator=(const type_factory &) noexcept = default;
			constexpr type_factory(type_factory &&) noexcept = default;
			constexpr type_factory &operator=(type_factory &&) noexcept = default;

			// clang-format off
			/** Adds `P` to the list of parents of `T`. */
			template<typename P>
			type_factory &parent() requires std::derived_from<T, P> && std::same_as<std::remove_cvref_t<P>, P>
			{
				data.template add_parent<T, P>();
				return *this;
			}
			/** Adds an attribute to `T`'s list of attributes. */
			template<typename A, typename... Args>
			type_factory<T, A, Attr...> attrib(Args &&...args)
			{
				data.template add_attrib<T>(type_seq<A, Attr...>, [&](){ return make_any<A>(std::forward<Args>(args)...); });
				return type_factory<T, A, Attr...>{data};
			}
			/** Adds an attribute to `T`'s list of attributes. */
			template<auto Value, typename A = std::remove_cvref_t<decltype(Value)>>
			type_factory<T, A, Attr...> attrib()
			{
				data.template add_attrib<T>(type_seq<A, Attr...>, [](){ return forward_any(auto_constant<Value>::value); });
				return type_factory<T, A, Attr...>{data};
			}
			// clang-format on

		private:
			data_t &data;
		};

		/** @brief Structure used to represent information about a parent-child relationship between reflected types. */
		class parent_info
		{
			friend class data_node_iterator<parent_info>;
			friend class type_info;

			constexpr parent_info() noexcept = default;
			constexpr explicit parent_info(const data_t::parent_node *node) noexcept : node(node) {}

		public:
			constexpr parent_info(const parent_info &) noexcept = default;
			constexpr parent_info &operator=(const parent_info &) noexcept = default;
			constexpr parent_info(parent_info &&) noexcept = default;
			constexpr parent_info &operator=(parent_info &&) noexcept = default;

			/** Returns type info of the parent type. */
			[[nodiscard]] constexpr type_info type() const noexcept { return type_info{node->type}; }

			/** Casts an `any` instance containing an object (or reference to one) of child type to
			 * an `any` instance of parent type (preserving const-ness).
			 * @note Passed `any` instance must be a reference. Passing a non-reference `any` will result in
			 * undefined behavior (likely a crash). */
			[[nodiscard]] any cast(any child) const;

			[[nodiscard]] constexpr bool operator==(const parent_info &) const noexcept = default;

		private:
			const data_t::parent_node *node = nullptr;
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
		friend class parent_info;

		using parent_iterator = data_node_iterator<parent_info>;

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

		/** Returns a range of parents of this type. */
		[[nodiscard]] constexpr data_node_view<parent_iterator> parents() const noexcept
		{
			return {parent_iterator{parent_info{data->parents.front}}, {}};
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

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			return data == other.data || name() == other.name();
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
		struct vtable_t
		{
			void (*copy_construct)(any &, const any &);
			void (*copy_assign)(any &, const any &);
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

		any(const any &other)
		{
			if (other.vtable) [[likely]]
				other.vtable->copy_construct(*this, other);
		}
		any &operator=(const any &other)
		{
			if (this != &other) [[likely]]
			{
				if (empty() && other.vtable) [[unlikely]]
					other.vtable->copy_construct(*this, other);
				else if (other.vtable) [[likely]]
					other.vtable->copy_assign(*this, other);
				else
					reset_impl();
			}
			return *this;
		}
		~any() { reset_impl(); }

		/** Initializes `any` with the managed object direct-initialized from `value`. */
		template<typename T>
		any(T &&value) noexcept : any(std::in_place_type<T>, std::forward<T>(value))
		{
		}
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

		/** Returns type info of the stored object. */
		[[nodiscard]] constexpr type_info type() const noexcept { return info; }

		/** Checks if `any` manages an object or a reference. */
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

		/** Returns raw pointer to the managed or referenced object's data.
		 * @note If the managed or referenced object is const-qualified, returns nullptr. */
		[[nodiscard]] constexpr void *data() noexcept
		{
			return is_const() ? nullptr : is_local() ? storage.local.data() : storage.external;
		}
		/** Returns raw const pointer to the managed or referenced object's data. */
		[[nodiscard]] constexpr const void *cdata() const noexcept
		{
			return is_local() ? storage.local.data() : storage.external;
		}
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return cdata(); }

		/** Returns an instance of `any` referencing the managed object. */
		[[nodiscard]] any ref() const noexcept
		{
			return any{vtable, info, const_cast<void *>(data()), static_cast<flags_t>(IS_REF | (flags & IS_CONST))};
		}

		/** Returns pointer to the managed or referenced object as `T` pointer.
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
		/** Returns const pointer to the managed or referenced object as `T` pointer.
		 * @note `T` must be the same as the underlying object.
		 * @return Pointer to the underlying object or nullptr if the underlying object is of a different type. */
		template<typename T>
		[[nodiscard]] constexpr const std::remove_const_t<T> *as_cptr() const noexcept
		{
			using U = std::remove_const_t<T>;
			if (info == type_info::get<U>()) [[likely]]
				return static_cast<const U *>(cdata());
			return nullptr;
		}
		/** @copydoc as_ptr */
		template<typename T>
		[[nodiscard]] constexpr const std::remove_const_t<T> *as_ptr() const noexcept
		{
			return as_cptr<T>();
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
			if (vtable != nullptr) [[likely]]
				vtable->destroy(*this);
		}

		const vtable_t *vtable = nullptr;
		type_info info;
		storage_t storage;
		flags_t flags = {};
	};

	template<typename T>
	constinit const any::vtable_t any::vtable_instance<T>::value = {
		.copy_construct = +[](any &to, const any &from) -> void
		{
			to.vtable = from.vtable;
			to.info = from.info;
			to.storage = storage_t(std::in_place_type<T>, *static_cast<const T *>(from.data()));
			to.flags = make_flags<T>();
		},
		.copy_assign = +[](any &to, const any &from) -> void
		{
			constexpr auto reset_copy = [](any &t, const any &f)
			{
				t.reset();
				t.info = f.info;
				t.storage = storage_t(std::in_place_type<T>, *static_cast<const T *>(f.data()));
			};

			if constexpr (std::is_const_v<T> || !std::is_copy_assignable_v<T>)
				reset_copy(to, from);
			else if (to.info != from.info)
				reset_copy(to, from);
			else if constexpr (local_candidate<T>)
				*to.storage.local.template get<T>() = *static_cast<const T *>(from.data());
			else
				*static_cast<T *>(to.storage.external) = *static_cast<const T *>(from.data());
			to.vtable = from.vtable;
			to.flags = make_flags<T>();
		},
		.destroy = +[](any &instance) -> void
		{
			/* References are not destroyed. */
			if (instance.flags & IS_REF) [[unlikely]]
				return;

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
			constexpr attrib_node() noexcept = default;

			any data;
		};

		template<typename T>
		constexpr decltype(auto) unwrap_any_arg(type_selector_t<T>, any &a)
		{
			using U = std::remove_reference_t<T>;
			if constexpr (std::is_const_v<U>)
				return std::forward<T>(*a.as_cptr<U>());
			else
			{
				SEK_ASSERT(!a.is_const(), "Cannot bind const `any` to non-const argument");
				return std::forward<T>(*a.as_ptr<U>());
			}
		}

		template<typename T, typename... Args>
		constexpr type_data::ctor_node make_type_ctor() noexcept
		{
			// clang-format off
			constexpr auto &arg_ts = auto_constant<std::array<type_handle, sizeof...(Args)>{type_handle{type_selector<Args>}...}>::value;
			// clang-format on
			return type_data::ctor_node{
				{arg_ts},
				+[](any obj, std::span<any> args)
				{
					constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, T * p, std::span<any> & as)
					{
						if constexpr (sizeof...(Args) == 0 || std::is_aggregate_v<T>)
							new (p) T{unwrap_any_arg(type_selector<type_seq_element_t<Is, type_seq_t<Args...>>>, as[Is])...};
						else
							new (p) T(unwrap_any_arg(type_selector<type_seq_element_t<Is, type_seq_t<Args...>>>, as[Is])...);
					};

					SEK_ASSERT(!obj.is_const(), "Cannot placement construct a const `any`");
					SEK_ASSERT(obj.is_ref(), "Cannot placement construct a non-reference `any`");

					unwrap(std::make_index_sequence<sizeof...(Args)>(), obj.template as_ptr<T>(), args);
				},
				+[](std::span<any> args) -> any
				{
					constexpr auto unwrap = []<std::size_t... Is>(std::index_sequence<Is...>, std::span<any> & as)
					{
						return make_any<T>(
							unwrap_any_arg(type_selector<type_seq_element_t<Is, type_seq_t<Args...>>>, as[Is])...);
					};
					return unwrap(std::make_index_sequence<sizeof...(Args)>(), args);
				},
			};
		}
		template<typename T>
		constinit const type_data::ctor_node type_data::ctor_node::default_instance<T>::value = make_type_ctor<T>();

		template<typename T, typename... Args>
		void type_data::add_ctor() noexcept
		{
			if constexpr (sizeof...(Args) != 0) /* Default constructor is automatically added. */
			{
				constinit static auto node = make_type_ctor<T, Args...>();
				if (!node.next) [[likely]]
					ctors.insert(node);
			}
		}
		template<typename T, typename P>
		void type_data::add_parent() noexcept
		{
			constinit static parent_node node{
				type_handle(type_selector<P>),
				+[](any child) -> any
				{
					SEK_ASSERT(child.is_ref(), "Cannot cast by-value `any`");

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
			using ctor_node = type_data::ctor_node;
			using ctor_list = type_data::node_list<ctor_node>;

			return type_data{
				.name = type_name<T>(),
				.extent = std::is_bounded_array_v<T> ? std::extent_v<T> : 0,
				.value_type = type_handle{select_value_type<T>()},
				.flags = make_type_flags<T>(),
				.dtor =
					+[](any obj)
					{
						if constexpr (std::is_destructible_v<T>)
						{
							SEK_ASSERT(!obj.is_const(), "Cannot placement destroy a const `any`");
							SEK_ASSERT(obj.is_ref(), "Cannot placement destroy a non-reference `any`");
							if constexpr (std::is_array_v<T>)
								delete[] obj.template as_ptr<T>();
							else
								delete obj.template as_ptr<T>();
						}
					},
				.ctors = std::is_default_constructible_v<T> ? ctor_list{&ctor_node::default_instance<T>::value} : ctor_list{},
			};
		}
	}	 // namespace detail

	any type_info::parent_info::cast(any child) const { return node->cast(std::move(child)); }
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
