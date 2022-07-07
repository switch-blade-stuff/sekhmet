/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/dynamic_buffer_resource.hpp"

#include "util.hpp"

namespace sek::serialization
{
	/** @brief Template for value nodes for use with `basic_node_tree`.
	 * @note Value nodes must be trivial types and provide a public `type_selector` member typedef,
	 * used to store the type of the value node. */
	template<typename... Ts>
	class basic_value_node;

	namespace detail
	{
		template<typename Node, typename Key>
		struct container_element_base
		{
			Node value;
			Key key;
		};
		template<typename Node>
		struct container_element_base<Node, void>
		{
			Node value;
		};
	}	 // namespace detail

	/** @brief Structure used to store a serialized node tree.
	 *
	 * Node trees are used to implement structured serialization archives
	 * and to transfer serialized data between compatible archive formats.
	 *
	 * For example, all Json-based archives (Json, & UBJson) use the `json_node_tree` specialization of `basic_node_tree`.
	 *
	 * @tparam C Character type used for node & attribute keys.
	 * @tparam N Type of value nodes stored within the tree.
	 * @tparam A Type of attributes stored within the tree.
	 * @tparam T Traits type of `C`.
	 *
	 * @note If attribute type is set to `void`, tree nodes will not have attributes.
	 * @note Node trees do not provide full serialization functionality themselves,
	 * they only act as serialized data storage. */
	template<typename C, typename N, typename A, typename T = std::char_traits<C>>
	class basic_node_tree
	{
		static_assert(std::is_trivially_copyable_v<N>);
		static_assert(std::is_trivially_copyable_v<A> || std::is_void_v<A>);

	public:
		typedef C char_type;
		typedef T traits_type;
		typedef std::basic_string_view<C, T> key_type;

		typedef N value_node;

		/** @brief Structure containing value & storage type selectors. */
		struct type_selector
		{
			/** Value data type selector. */
			typename N::type_selector value = {};
			/** Storage type selector. */
			enum : std::uint8_t
			{
				/** Node has a dynamic type (used for container types). */
				DYNAMIC,
				/** Node is a value node. */
				VALUE,
				/** Node is an array node. */
				ARRAY,
				/** Node is a table node. */
				TABLE,
			} storage = DYNAMIC;

			[[nodiscard]] constexpr bool operator==(const type_selector &) const noexcept = default;
			[[nodiscard]] constexpr bool operator!=(const type_selector &) const noexcept = default;
		};

	private:
		template<typename>
		struct container_element;
		template<typename>
		class container_node;

		// clang-format off
		template<typename E>
		constexpr static bool has_key = requires(E e) { e.key; };
		// clang-format on

		template<typename Element>
		class node_iterator
		{
			template<typename>
			friend class container_node;

		public:
			typedef Element value_type;
			typedef Element *pointer;
			typedef Element &reference;
			typedef std::ptrdiff_t difference_type;

			typedef std::random_access_iterator_tag iterator_category;

			/** `true` if the pointed-to entry has a key, `false` otherwise. */
			constexpr static bool has_key = basic_node_tree::has_key<Element>;

		private:
			constexpr explicit node_iterator(value_type *ptr) noexcept : m_ptr(ptr) {}

		public:
			constexpr node_iterator() noexcept = default;
			constexpr node_iterator(const node_iterator &) noexcept = default;
			constexpr node_iterator &operator=(const node_iterator &) noexcept = default;
			constexpr node_iterator(node_iterator &&) noexcept = default;
			constexpr node_iterator &operator=(node_iterator &&) noexcept = default;

			constexpr node_iterator &operator+=(difference_type n) noexcept
			{
				m_ptr += n;
				return *this;
			}
			constexpr node_iterator &operator++() noexcept
			{
				++m_ptr;
				return *this;
			}
			constexpr node_iterator operator++(int) noexcept
			{
				auto temp = *this;
				operator++();
				return temp;
			}
			constexpr node_iterator &operator-=(difference_type n) noexcept
			{
				m_ptr -= n;
				return *this;
			}
			constexpr node_iterator &operator--() noexcept
			{
				--m_ptr;
				return *this;
			}
			constexpr node_iterator operator--(int) noexcept
			{
				auto temp = *this;
				operator--();
				return temp;
			}

			[[nodiscard]] constexpr node_iterator operator+(difference_type n) const noexcept
			{
				auto result = *this;
				result.m_ptr += n;
				return result;
			}
			[[nodiscard]] constexpr node_iterator operator-(difference_type n) const noexcept
			{
				auto result = *this;
				result.m_ptr -= n;
				return result;
			}

			/** Returns pointer to the associated entry. */
			[[nodiscard]] constexpr pointer get() const noexcept { return m_ptr; }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the associated entry. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
			/** Returns reference to the entry at `n` offset from the iterator. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return get()[n]; }

			/** Returns reference to the type of the pointed-to node. */
			[[nodiscard]] constexpr auto &type() const noexcept;
			/** Returns reference to the value of the pointed-to node. */
			[[nodiscard]] constexpr auto &value() const noexcept;
			/** Returns reference to the key of the pointed-to node.
			 * @note This function is available only for keyed entries. */
			[[nodiscard]] constexpr auto &key() const noexcept;

			[[nodiscard]] friend constexpr difference_type operator-(node_iterator a, node_iterator b) noexcept
			{
				return a.m_ptr - b.m_ptr;
			}
			[[nodiscard]] friend constexpr node_iterator operator+(difference_type n, node_iterator a) noexcept
			{
				return a + n;
			}

			[[nodiscard]] constexpr auto operator<=>(const node_iterator &other) const noexcept
			{
				return m_ptr <=> other.m_ptr;
			}
			[[nodiscard]] constexpr bool operator==(const node_iterator &other) const noexcept
			{
				return m_ptr == other.m_ptr;
			}

		private:
			value_type *m_ptr;
		};
		template<typename Element>
		class container_node
		{
			template<typename, typename, typename, typename>
			friend class basic_node_tree;

		public:
			typedef node_iterator<Element> iterator;
			typedef node_iterator<const Element> const_iterator;

			typedef typename iterator::value_type value_type;
			typedef typename iterator::pointer pointer;
			typedef typename iterator::reference reference;
			typedef typename const_iterator::pointer const_pointer;
			typedef typename const_iterator::reference const_reference;
			typedef typename iterator::difference_type difference_type;
			typedef std::size_t size_type;

		public:
			/** Initializes an empty container node. */
			constexpr container_node() noexcept = default;

			/** Returns iterator to the first entry of the container node. */
			[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_data}; }
			/** Returns iterator to the first entry of the container node. */
			[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{m_data}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last entry of the container node. */
			[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_data + m_size}; }
			/** Returns iterator one past the last entry of the container node. */
			[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_data + m_size}; }
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

			/** Returns reference to the first entry of the container node. */
			[[nodiscard]] constexpr reference front() noexcept { return *begin(); }
			/** @copydoc front */
			[[nodiscard]] constexpr const_reference front() const noexcept { return *begin(); }
			/** Returns reference to the last entry of the container node. */
			[[nodiscard]] constexpr reference back() noexcept { return *(end() - 1); }
			/** @copydoc back */
			[[nodiscard]] constexpr const_reference back() const noexcept { return *(end() - 1); }
			/** Returns reference to the nth entry of the container node. */
			[[nodiscard]] constexpr reference at(size_type i) noexcept
			{
				return begin()[static_cast<difference_type>(i)];
			}
			/** @copydoc at */
			[[nodiscard]] constexpr const_reference at(size_type i) const noexcept
			{
				return begin()[static_cast<difference_type>(i)];
			}

			/** Inserts a node at the specified location within the container.
			 * If there is not enough space (`capacity() <= size()`, returns `false`), otherwise returns `true`. */
			constexpr bool insert(const_iterator where, value_type &&node) noexcept
			{
				if (capacity() <= size()) [[unlikely]]
					return false;
				else
				{
					auto old_pos = m_data + (where - cbegin());
					const auto old_size = m_size++;

					std::move_backward(old_pos, m_data + old_size, m_data + m_size);
					*old_pos = std::move(node);
					return true;
				}
			}
			/** @copydoc insert */
			constexpr bool insert(const_iterator where, const value_type &node) noexcept
			{
				if (capacity() <= size()) [[unlikely]]
					return false;
				else
				{
					auto old_pos = m_data + (where - cbegin());
					const auto old_size = m_size++;

					std::move_backward(old_pos, m_data + old_size, m_data + m_size);
					*old_pos = node;
					return true;
				}
			}

			/** Checks if the container is empty. */
			[[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
			/** Returns the size of the container node. */
			[[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
			/** Returns the capacity of the container node. */
			[[nodiscard]] constexpr size_type capacity() const noexcept { return m_capacity; }

		private:
			value_type *m_data = nullptr;
			size_type m_size = 0;
			size_type m_capacity = 0;

		public:
			/** Type of container elements. `type_selector::DYNAMIC` by default. */
			type_selector element_type;
		};

	public:
		/** @brief Container node used to store un-keyed entries. */
		typedef container_node<container_element<void>> array_node;
		/** @brief Container node used to store keyed entries. */
		typedef container_node<container_element<key_type>> table_node;
		/** @brief Storage for container & value nodes. */
		class node_type
		{
			template<typename, typename, typename, typename>
			friend class basic_node_tree;

		public:
			/** Initializes an empty value node. */
			constexpr node_type() noexcept : m_value{} {}

			/** Returns reference to the data of a value node. */
			[[nodiscard]] constexpr auto &value() noexcept { return m_value; }
			/** @copydoc value */
			[[nodiscard]] constexpr auto &value() const noexcept { return m_value; }
			/** Returns reference to the data of an array node. */
			[[nodiscard]] constexpr auto &array() noexcept { return m_array; }
			/** @copydoc array */
			[[nodiscard]] constexpr auto &array() const noexcept { return m_array; }
			/** Returns reference to the data of a table node. */
			[[nodiscard]] constexpr auto &table() noexcept { return m_table; }
			/** @copydoc table */
			[[nodiscard]] constexpr auto &table() const noexcept { return m_table; }

			/** Checks if the node is a value node. */
			[[nodiscard]] constexpr bool is_value() const noexcept { return type.storage == type_selector::VALUE; }
			/** Checks if the node is an array node. */
			[[nodiscard]] constexpr bool is_array() const noexcept { return type.storage == type_selector::ARRAY; }
			/** Checks if the node is a table node. */
			[[nodiscard]] constexpr bool is_table() const noexcept { return type.storage == type_selector::TABLE; }

			/** Sets the node to value storage & returns reference to the node. */
			constexpr auto &to_value() noexcept
			{
				type.storage = type_selector::VALUE;
				m_value = {};
				return *this;
			}
			/** Sets the node to array storage & returns reference to the node. */
			constexpr auto &to_array() noexcept
			{
				type.storage = type_selector::ARRAY;
				m_array = {};
				return *this;
			}
			/** Sets the node to table storage & returns reference to the node. */
			constexpr auto &to_table() noexcept
			{
				type.storage = type_selector::TABLE;
				m_table = {};
				return *this;
			}

			constexpr void swap(node_type &other) noexcept
			{
				std::swap(type, other.type);
				std::swap(padding, other.padding);
			}

			/** Storage & data type of the node. */
			type_selector type;

		private:
			union
			{
				std::byte padding[sizeof(value_node) > sizeof(array_node) ? sizeof(value_node) : sizeof(array_node)];
				value_node m_value; /* Data of a value node. */
				array_node m_array; /* Data of an array node. */
				table_node m_table; /* Data of a table node. */
			};
		};

	private:
		using node_pool_t = sek::detail::dynamic_buffer_resource<sizeof(node_type) * 64>;
		using string_pool_t = sek::detail::dynamic_buffer_resource<SEK_KB(8)>;

		template<typename K>
		struct container_element : detail::container_element_base<node_type, K>
		{
		};

	public:
		constexpr static auto has_attribute = !std::same_as<A, void>;

		explicit basic_node_tree(std::pmr::memory_resource *res) noexcept : string_pool(res), node_pool(res) {}

		/** Allocates a string (n + 1 chars) using the string pool.
		 * @throw std::bad_alloc on allocation failure. */
		[[nodiscard]] constexpr char_type *alloc_string(std::size_t n)
		{
			auto result = static_cast<char_type *>(string_pool.allocate((n + 1) * sizeof(char_type)));
			if (!result) [[unlikely]]
				throw std::bad_alloc();
			return result;
		}
		/** Copies a string (n + 1 chars) using the string pool.
		 * @throw std::bad_alloc on allocation failure. */
		[[nodiscard]] constexpr std::basic_string_view<C, T> copy_string(const char_type *c, std::size_t n)
		{
			auto dst = alloc_string(n);
			*std::copy_n(c, n, dst) = '\0';
			return {dst, n};
		}
		/** @copydoc copy_string */
		template<typename U = T>
		[[nodiscard]] constexpr std::basic_string_view<C, T> copy_string(std::basic_string_view<C, U> str)
		{
			return copy_string(str.data(), str.size());
		}
		/** Generates a key string from an index using the string pool.
		 * @tparam Prefix Prefix of the generated key.
		 * @param idx Index to use for generating the key.
		 * @throw std::bad_alloc on allocation failure. */
		template<basic_static_string Prefix = "__">
		[[nodiscard]] constexpr std::basic_string_view<C, T> make_key(std::size_t idx)
		{
			return detail::generate_key<C, T, Prefix>(string_pool, idx);
		}

		/** Resizes a container node to the specified capacity.
		 * @param node Container node to resize.
		 * @param n New capacity of the container node.
		 * @return Reference to the container node.
		 * @throw std::bad_alloc on allocation failure. */
		template<typename E>
		[[nodiscard]] constexpr auto &reserve_container(container_node<E> &node, std::size_t n)
		{
			if (n > node.capacity()) [[likely]]
			{
				const auto old_bytes = node.capacity() * sizeof(E);
				const auto new_bytes = n * sizeof(E);

				auto new_data = node_pool.reallocate(node.m_data, old_bytes, new_bytes, alignof(E));
				if (!new_data) [[unlikely]]
					throw std::bad_alloc();

				node.m_data = static_cast<E *>(new_data);
				node.m_capacity = n;
			}
			return node;
		}

		void reset(std::pmr::memory_resource *res)
		{
			top_level = node_type{};
			string_pool = string_pool_t{res};
			node_pool = string_pool_t{res};
		}
		void reset()
		{
			top_level = node_type{};
			string_pool.release();
			node_pool.release();
		}

		constexpr void swap(basic_node_tree &other) noexcept
		{
			top_level.swap(other.top_level);
			string_pool.swap(other.string_pool);
			node_pool.swap(other.node_pool);
		}
		friend constexpr void swap(basic_node_tree &a, basic_node_tree &b) noexcept { a.swap(b); }

	public:
		/** Top-most node of the node tree. */
		node_type top_level = {};
		/** Allocation pool used for string allocation. */
		string_pool_t string_pool;
		/** Allocation pool used for node allocation. */
		node_pool_t node_pool;
	};

	template<typename C, typename N, typename A, typename T>
	template<typename E>
	constexpr auto &basic_node_tree<C, N, A, T>::node_iterator<E>::type() const noexcept
	{
		return m_ptr->value.type;
	}
	template<typename C, typename N, typename A, typename T>
	template<typename E>
	constexpr auto &basic_node_tree<C, N, A, T>::node_iterator<E>::value() const noexcept
	{
		return m_ptr->value;
	}
	template<typename C, typename N, typename A, typename T>
	template<typename E>
	constexpr auto &basic_node_tree<C, N, A, T>::node_iterator<E>::key() const noexcept
	{
		return m_ptr->key;
	}
}	 // namespace sek::serialization