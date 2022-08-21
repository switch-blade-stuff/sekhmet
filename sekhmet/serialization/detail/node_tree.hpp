/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/buffer_allocator.hpp"

#include "manipulators.hpp"
#include "util.hpp"

namespace sek::serialization
{
	template<typename, typename, typename = void>
	class basic_node_tree;

	/** @brief Helper structure used to select conversion function of a `node_type` overload. */
	template<typename T>
	struct convert_from_t
	{
	};
	/** @copydoc convert_from_t */
	template<typename T>
	struct convert_to_t
	{
	};

	/** @brief Structure used to pass node (de)serialization context to specializations of `node_type`. */
	class node_context
	{
		template<typename, typename, typename>
		friend class basic_node_tree;

	private:
		using allocator_type = sek::detail::buffer_allocator<SEK_KB(8)>;

		constexpr explicit node_context(allocator_type &alloc) noexcept : m_alloc(alloc) {}

	public:
		node_context() = delete;
		node_context(const node_context &) = delete;
		node_context &operator=(const node_context &) = delete;
		node_context(node_context &&) = delete;
		node_context &operator=(node_context &&) = delete;

		/** Allocates `n` contiguous objects of type `T`. */
		template<typename T>
		[[nodiscard]] constexpr T *allocate(std::size_t n) const
		{
			return allocate<T>(n, std::align_val_t{alignof(T)});
		}
		/** @copydoc allocate */
		template<typename T>
		[[nodiscard]] constexpr T *allocate(std::size_t n, std::align_val_t align) const
		{
			return static_cast<T *>(m_alloc.allocate(n * sizeof(T), align));
		}

		/** Reallocates a previously allocated chunk of memory. */
		template<typename T>
		[[nodiscard]] constexpr T *reallocate(T *ptr, std::size_t old_n, std::size_t n) const
		{
			return reallocate<T>(ptr, old_n, n, std::align_val_t{alignof(T)});
		}
		/** @copydoc reallocate */
		template<typename T>
		[[nodiscard]] constexpr T *reallocate(T *ptr, std::size_t old_n, std::size_t n, std::align_val_t align) const
		{
			return static_cast<T *>(m_alloc.reallocate(ptr, old_n * sizeof(T), n * sizeof(T), align));
		}

		/** Deallocates a previously allocated chunk of memory. */
		template<typename T>
		constexpr void deallocate(T *ptr, std::size_t n) const
		{
			dereallocate<T>(ptr, n, std::align_val_t{alignof(T)});
		}
		/** @copydoc deallocate */
		template<typename T>
		constexpr void deallocate(T *ptr, std::size_t n, std::align_val_t align) const
		{
			m_alloc.deallocate(ptr, n * sizeof(T), align);
		}

	private:
		allocator_type &m_alloc;
	};

	/** @brief Template used for defining separate data types for use with `basic_node_tree`.
	 *
	 * Specializations of this template must provide member `operator()` functions, used to preform
	 * overload resolution of serialization functions and conversion between data types. If a `node_type`
	 * template instance does not have an available operator, it is ignored.
	 *
	 * Every `operator()` receives an instance of `node_context` that may be used to preform additional memory
	 * allocations required for node type conversion (for example to handle allocation of strings).
	 *
	 * @tparam Name Name of the node value used for debugging purposes.
	 * @tparam T Type of the node value.
	 *
	 * @example
	 * @code{.cpp}
	 * template<>
	 * struct node_type<"int32", std::int32_t>
	 * {
	 * 	template<typename T>
	 * 	constexpr static bool valid_int = std::signed_integral<T> && sizeof(T) == sizeof(std::int32_t);
	 *
	 * 	template<typename I>
	 * 	[[nodiscard]] constexpr std::int32_t operator()(node_context, convert_from_t<I>, I i, auto &&...) requires valid_int<I>
	 * 	{
	 * 		return static_cast<std::int32_t>(i);
	 * 	}
	 * 	template<typename I>
	 * 	[[nodiscard]] constexpr I operator()(node_context, convert_to_t<I>, std::int32_t i, auto &&...) requires std::is_convertible_v<std::int32_t, I>
	 * 	{
	 * 		return static_cast<I>(i);
	 * 	}
	 * };
	 *
	 * template<>
	 * struct node_type<"uint32",std::uint32_t>
	 * {
	 * 	template<typename T>
	 * 	constexpr static bool valid_int = std::unsigned_integral<T> && sizeof(T) == sizeof(std::uint32_t);
	 *
	 * 	template<typename I>
	 * 	[[nodiscard]] constexpr std::uint32_t operator()(node_context, convert_from_t<I>, I i, auto &&...) requires valid_int<I>
	 * 	{
	 * 		return static_cast<std::uint32_t>(i);
	 * 	}
	 * 	template<typename I>
	 * 	[[nodiscard]] constexpr I operator()(node_context, convert_to_t<I>, std::uint32_t i, auto &&...) requires std::is_convertible_v<std::uint32_t, I>
	 * 	{
	 * 		return static_cast<I>(i);
	 * 	}
	 * };
	 * @endcode */
	template<basic_static_string Name, typename T>
	struct node_type;

	/** @brief Template for value nodes for use with `basic_node_tree`.
	 * @note Value nodes must be trivial types and provide a public `type_selector` member typedef,
	 * used to store the type of the value node. */
	template<typename... Ts>
	class basic_value_node;

	namespace detail
	{
		template<typename A>
		struct attribute_typedef
		{
			typedef A attribute_type;
		};
		template<>
		struct attribute_typedef<void>
		{
		};

		template<typename>
		struct node_traits;
		template<basic_static_string Name, typename T>
		struct node_traits<node_type<Name, T>>
		{
			using type = T;

			constexpr static auto name = Name;
		};
		template<typename N>
		using node_value_type_t = typename node_traits<N>::type;

		// clang-format off
		/* To convert to a node's value type, we check if a `convert_from_t` overload exists that returns node's value type. */
		template<typename N, typename T, typename... Args>
		concept convertible_to_node = std::is_invocable_r_v<node_value_type_t<N>, N, convert_from_t<std::remove_cvref_t<T>>, T, Args...>;
		/* To convert from a node's value type, we check if a `convert_to_t` overload exists that returns the desired type. */
		template<typename N, typename T, typename... Args>
		concept convertible_from_node = std::is_invocable_r_v<T, N, convert_to_t<std::remove_cvref_t<T>>, node_value_type_t<N>, Args...>;
		// clang-format on

		/* To check if a node value cast is available, we check if the converted-to node's value type is convertible
		 * from the converted-from node's value type. */
		template<typename NFrom, typename NTo>
		concept valid_node_cast = convertible_from_node<NFrom, node_value_type_t<NTo>>;

		template<typename...>
		class node_data;
		template<>
		class node_data<>
		{
		};
		template<typename T, typename... Ts>
		class node_data<T, Ts...> : ebo_base_helper<node_value_type_t<T>>, public node_data<Ts...>
		{
			using base_t = ebo_base_helper<node_value_type_t<T>>;

		public:
			constexpr static auto id = T::id;

			using base_t::get;
		};

		template<typename... Ts>
		class node_storage
		{
			template<auto Id, typename U, typename... Us>
			constexpr static auto *cast_data(node_data<U, Us...> *data) noexcept
			{
				if constexpr (node_data<U, Us...>::id != Id)
					return cast_data<Id>(static_cast<node_data<Us...> *>(data));
				else
					return data;
			}
			template<auto Id, typename U, typename... Us>
			constexpr static auto *cast_data(const node_data<U, Us...> *data) noexcept
			{
				if constexpr (node_data<U, Us...>::id != Id)
					return cast_data<Id>(static_cast<const node_data<Us...> *>(data));
				else
					return data;
			}

		public:
			constexpr node_storage() noexcept = default;

			template<typename N>
			[[nodiscard]] constexpr node_value_type_t<N> *get() noexcept
			{
				return get<N::id>();
			}
			template<typename N>
			[[nodiscard]] constexpr const node_value_type_t<N> *get() const noexcept
			{
				return get<N::id>();
			}

			constexpr void swap(node_storage &other) noexcept { std::swap(m_data, other.m_data); }

		private:
			type_storage<node_data<Ts...>> m_data;
		};

		template<typename N, typename K, typename A>
		struct container_item_base;
		template<typename N>
		struct container_item_base<N, void, void>
		{
			N value;
		};
		template<typename N, typename K>
		struct container_item_base<N, void, K>
		{
			N value;
			K key;
		};
		template<typename N, typename A>
		struct container_item_base<N, A, void>
		{
			A attribute;
			N value;
		};
		template<typename N, typename A, typename K>
		struct container_item_base
		{
			A attribute;
			N value;
			K key;
		};
	}	 // namespace detail

	/** @brief Serialization archive used to store a serialized node tree.
	 *
	 * Node trees are used as a base to implement structured serialization archives and to transfer serialized data
	 * between compatible archive formats. A specialized archive should derive from the node tree and implement custom
	 * data format parsing or generation.
	 *
	 * For example, all Json-based archives (Json, & UBJson) use the `json_node_tree` specialization of `basic_node_tree`.
	 *
	 * @tparam Nodes Specializations of the `node_type` template, used to specify individual data types of tree's nodes.
	 * @tparam C Character type used to store container keys.
	 * @tparam A Type of attribute value assigned to each node. If set to `void`, nodes do not have attributes.
	 *
	 * @note Node value types must be trivially copyable. */
	template<typename... Nodes, typename C, typename A>
	class basic_node_tree<type_seq_t<Nodes...>, C, A> : public detail::attribute_typedef<A>
	{
		static_assert((std::is_trivially_copyable_v<detail::node_value_type_t<Nodes>> && ...),
					  "Node value types must be trivially copyable");

		template<typename E>
		constexpr static bool has_attribute = requires(E e) { e.attribute; };
		template<typename E>
		constexpr static bool has_key = requires(E e) { e.key; };

		using node_data_t = detail::node_storage<Nodes...>;
		using key_type = std::basic_string_view<C>;
		using id_type = std::uint32_t;

		template<typename T>
		constexpr static id_type node_id = type_seq_index_v<T, type_seq_t<Nodes...>> + 1;

		constexpr static id_type dynamic_id = (4u << (std::numeric_limits<id_type>::digits - 4)); /* No fixed type. */
		constexpr static id_type object_id = (2u << (std::numeric_limits<id_type>::digits - 4)); /* Node is an object. */
		constexpr static id_type array_id = (1u << (std::numeric_limits<id_type>::digits - 4));	 /* Node is an array. */

		constexpr static id_type container_id = array_id | object_id; /* Node is an object. */
		constexpr static id_type empty_id = 0;						  /* Node is empty. */

	public:
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		constexpr static size_type buffer_size = SEK_KB(8);

		class value_node;
		template<typename Element>
		struct container_data
		{
			size_type size = 0;
			size_type capacity = 0;
			Element *data = nullptr;
		};

		using array_element = container_data<detail::container_item_base<value_node, void, A>>;
		using array_data = container_data<array_element>;
		using object_element = container_data<detail::container_item_base<value_node, key_type, A>>;
		using object_data = container_data<object_element>;

		class value_node
		{
			constexpr static auto pad_size = std::max(sizeof(node_data_t), std::max(sizeof(array_data), sizeof(object_data)));

		public:
			constexpr value_node() noexcept = default;
			constexpr ~value_node() noexcept = default;

			constexpr value_node(const value_node &other) : m_type_id(other.m_type_id), m_padding(other.m_padding) {}
			constexpr value_node &operator=(const value_node &other)
			{
				if (this != &other)
				{
					m_type_id = other.m_type_id;
					m_padding = other.m_padding;
				}
				return *this;
			}

			constexpr value_node(value_node &&other) noexcept { swap(other); }
			constexpr value_node &operator=(value_node &&other) noexcept
			{
				swap(other);
				return *this;
			}

			/** Checks if the node is empty (does not contain any data). */
			[[nodiscard]] constexpr bool empty() const noexcept { return m_type_id == empty_id; }
			/** Checks if the node is an array container. */
			[[nodiscard]] constexpr bool is_array() const noexcept { return m_type_id & array_id; }
			/** Checks if the node is an object container. */
			[[nodiscard]] constexpr bool is_object() const noexcept { return m_type_id & object_id; }
			/** Checks if the node is a container (array or object). */
			[[nodiscard]] constexpr bool is_container() const noexcept { return m_type_id & container_id; }
			/** Checks if the node is a container with dynamic type. */
			[[nodiscard]] constexpr bool is_dynamic() const noexcept { return m_type_id & dynamic_id; }

			/** Returns a 0-based index id of the `node_type` instance that corresponds with the current type of the node.
			 * If the node is a container node, it's type corresponds to the type of it's elements (unless `is_dynamic`
			 * evaluates to `true`). If the node is a value node, it's type corresponds to the type of the stored value.
			 * @note Result is only valid if the node is not empty. */
			[[nodiscard]] constexpr id_type type() const noexcept { return (m_type_id & ~container_id) - 1; }

			/** @brief Returns reference to the underlying value of a value node.
			 * @tparam N `node_type` corresponding to the contained value.
			 * @note Result is only valid if the node is not a container. */
			template<typename N>
			[[nodiscard]] constexpr auto &get() noexcept
			{
				return *m_value.template get<N>();
			}
			/** @copydoc get */
			template<typename N>
			[[nodiscard]] constexpr auto &get() const noexcept
			{
				return *m_value.template get<N>();
			}
			/** @copybrief get
			 * @tparam Type 0-based index corresponding to the `node_type` of the contained value.
			 * @note Result is only valid if the node is not a container. */
			template<id_type Type>
			[[nodiscard]] constexpr auto &get() noexcept
			{
				return get<type_seq_element_t<Type, type_seq_t<Nodes...>>>();
			}
			/** @copydoc get */
			template<id_type Type>
			[[nodiscard]] constexpr auto &get() const noexcept
			{
				return get<type_seq_element_t<Type, type_seq_t<Nodes...>>>();
			}

			/** Returns the size of a container (array or object) node.
			 * @note Result is only valid if the node is a container node. */
			[[nodiscard]] constexpr size_type size() const noexcept
			{
				return is_array() ? m_array.size : m_object.size;
			}
			/** Returns the capacity of a container (array or object) node.
			 * @note Result is only valid if the node is a container node. */
			[[nodiscard]] constexpr size_type capacity() const noexcept
			{
				return is_array() ? m_array.capacity : m_object.capacity;
			}

			constexpr void swap(value_node &other) noexcept
			{
				std::swap(m_padding, other.m_padding);
				std::swap(m_type_id, other.m_type_id);
			}
			friend constexpr void swap(value_node &a, value_node &b) noexcept { a.swap(b); }

		private:
			union
			{
				std::byte m_padding[pad_size] = {};
				node_data_t m_value;
				array_data m_array;
				array_data m_object;
			};
			id_type m_type_id = empty_id;
		};

		using node_allocator = sek::detail::buffer_allocator<std::max<size_type>(buffer_size / sizeof(value_node), 8)>;
		using buffer_allocator = sek::detail::buffer_allocator<buffer_size>;

	public:
		constexpr basic_node_tree() noexcept = default;

		/* TODO: Implement constructor taking a const node_type & which would traverse all nodes & copy their data. */
		/* TODO: Implement copy constructor & assignment that copy the top-level node. */
		/* TODO: Move generic implementation of archive frames to node tree,
		 * make node tree act as a basic non-parsing archive */

		constexpr basic_node_tree(basic_node_tree &&) noexcept = default;
		constexpr basic_node_tree &operator=(basic_node_tree &&) noexcept = default;

		/** Clears any serialized data and releases all allocated memory. */
		constexpr void reset()
		{
			m_top_level = value_node{};
			m_buffer_alloc.release();
			m_node_alloc.release();
		}

		constexpr void swap(basic_node_tree &other) noexcept
		{
			m_top_level.swap(other.m_top_level);
			m_buffer_alloc.swap(other.m_buffer_alloc);
			m_node_alloc.swap(other.m_node_alloc);
		}
		friend constexpr void swap(basic_node_tree &a, basic_node_tree &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr node_context make_context() noexcept { return node_context{m_buffer_alloc}; }

		/** Top-most node of the node tree. */
		value_node m_top_level = {};
		/** Allocator used for general buffer allocation. */
		buffer_allocator m_buffer_alloc;
		/** Allocator used for node allocation. */
		node_allocator m_node_alloc;
	};
}	 // namespace sek::serialization