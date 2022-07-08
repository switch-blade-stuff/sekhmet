/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <string>

#include "sekhmet/detail/access_guard.hpp"
#include "sekhmet/detail/basic_pool.hpp"
#include "sekhmet/detail/dense_set.hpp"
#include "sekhmet/detail/service.hpp"
#include "sekhmet/detail/type_info.hpp"
#include "sekhmet/serialization/json.hpp"
#include "sekhmet/system/native_file.hpp"

#include <shared_mutex>

namespace sek::engine
{
	class config_registry;

	namespace detail
	{
		using namespace sek::detail;

		using config_guard = access_guard<config_registry, std::shared_mutex>;
	}	 // namespace detail

	/** @brief Runtime exception thrown by the config registry. */
	class SEK_API config_error : public std::runtime_error
	{
	public:
		config_error() : std::runtime_error("Unknown config registry error") {}
		explicit config_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit config_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit config_error(const char *msg) : std::runtime_error(msg) {}
		~config_error() override;
	};

	/** @brief Path-like structure used to uniquely identify a config registry entry.
	 *
	 * Config paths consist of entry names separated by forward slashes `/`. The first entry is the category entry.
	 * Path entry names can only contain alphanumeric characters, whitespace, as well as any of the following
	 * special characters: `_`, `-`, `.`.
	 *
	 * Paths are case-sensitive and are always absolute (since there is no "current" config path).
	 * Sequential slashes (ex. `///`) are concatenated. */
	class cfg_path
	{
	private:
		struct slice_t
		{
			std::size_t first;
			std::size_t last;
		};

	public:
		/** Initializes an empty config path. */
		constexpr cfg_path() noexcept = default;

		/** Initializes a config path from a string view.
		 * @throw config_error If the specified path is invalid. */
		explicit cfg_path(std::string_view str) : m_path(str) { parse(); }
		/** Initializes a config path from a string.
		 * @throw config_error If the specified path is invalid. */
		explicit cfg_path(std::string &&str) : m_path(std::move(str)) { parse(); }
		/** @copydoc cfg_path */
		explicit cfg_path(const std::string &str) : m_path(str) { parse(); }

		/** Initializes a config path from a C-style string.
		 * @throw config_error If the specified path is invalid. */
		explicit cfg_path(const char *str) : cfg_path(std::string_view{str}) {}
		/** Initializes a config path from a character array.
		 * @throw config_error If the specified path is invalid. */
		cfg_path(const char *str, std::size_t n) : cfg_path(std::string_view{str, n}) {}

		/** Checks if the config path is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_slices.empty(); }
		/** Checks if the config path is a category path. */
		[[nodiscard]] constexpr bool is_category() const noexcept { return m_slices.size() == 1; }

		/** Returns reference to the underlying path string. */
		[[nodiscard]] constexpr auto &string() noexcept { return m_path; }
		/** @copydoc string */
		[[nodiscard]] constexpr auto &string() const noexcept { return m_path; }

		/** Returns the category component of the path. */
		[[nodiscard]] cfg_path category() const
		{
			auto ptr = &m_slices.back();
			return to_component(ptr, ptr + 1);
		}
		/** Returns the parent entry path. */
		[[nodiscard]] cfg_path parent_path() const
		{
			return to_component(m_slices.data(), m_slices.data() + m_slices.size() - 1);
		}
		/** Returns the entry path without the category component. */
		[[nodiscard]] cfg_path entry_path() const
		{
			return to_component(m_slices.data() + 1, m_slices.data() + m_slices.size());
		}
		/** Returns the last entry name of the path (ex. for path 'graphics/quality' will return `quality`). */
		[[nodiscard]] cfg_path entry_name() const
		{
			auto ptr = &m_slices.back();
			return to_component(ptr, ptr + 1);
		}

		/** Appends another path to this path.
		 * @return Reference to this path.
		 * @throw config_error If the resulting path is invalid. */
		cfg_path &append(const cfg_path &path)
		{
			m_path.append(path.m_path);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &operator/=(const cfg_path &path) { return append(path); }
		/** Returns a path produced from concatenating two paths.
		 * @return Concatenated path.
		 * @throw config_error If the resulting path is invalid. */
		[[nodiscard]] cfg_path operator/(const cfg_path &path) const
		{
			auto tmp = *this;
			tmp /= path;
			return tmp;
		}

		/** Appends a string to the path.
		 * @return Reference to this path.
		 * @throw config_error If the resulting path is invalid. */
		cfg_path &append(std::string_view str)
		{
			m_path.append(str);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &append(const std::string &str)
		{
			m_path.append(str);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &append(const char *str) { return append(std::string_view{str}); }
		/** @copydoc append */
		cfg_path &append(const char *str, std::size_t n) { return append(std::string_view{str, n}); }
		/** @copydoc append */
		cfg_path &operator/=(std::string_view str) { return append(std::string_view{str}); }
		/** @copydoc append */
		cfg_path &operator/=(const std::string &str) { return append(std::string_view{str}); }
		/** @copydoc append */
		cfg_path &operator/=(const char *str) { return append(std::string_view{str}); }

		/** Returns a path produced from concatenating a path with a string.
		 * @return Concatenated path.
		 * @throw config_error If the resulting path is invalid. */
		[[nodiscard]] cfg_path operator/(std::string_view str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}
		/** @copydoc operator/ */
		[[nodiscard]] cfg_path operator/(const std::string &str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}
		/** @copydoc operator/ */
		[[nodiscard]] cfg_path operator/(const char *str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}

		[[nodiscard]] constexpr auto operator<=>(const cfg_path &other) noexcept { return m_path <=> other.m_path; }
		[[nodiscard]] constexpr bool operator==(const cfg_path &other) noexcept { return m_path == other.m_path; }

		[[nodiscard]] friend constexpr auto operator<=>(const std::string &a, const cfg_path &b) noexcept
		{
			return a <=> b.m_path;
		}
		[[nodiscard]] friend constexpr bool operator==(const std::string &a, const cfg_path &b) noexcept
		{
			return a == b.m_path;
		}
		[[nodiscard]] friend constexpr auto operator<=>(const cfg_path &a, const std::string &b) noexcept
		{
			return a.m_path <=> b;
		}
		[[nodiscard]] friend constexpr bool operator==(const cfg_path &a, const std::string &b) noexcept
		{
			return a.m_path == b;
		}

		[[nodiscard]] friend constexpr auto operator<=>(std::string_view a, const cfg_path &b) noexcept
		{
			return a <=> b.m_path;
		}
		[[nodiscard]] friend constexpr bool operator==(std::string_view a, const cfg_path &b) noexcept
		{
			return a == b.m_path;
		}
		[[nodiscard]] friend constexpr auto operator<=>(const cfg_path &a, std::string_view b) noexcept
		{
			return a.m_path <=> b;
		}
		[[nodiscard]] friend constexpr bool operator==(const cfg_path &a, std::string_view b) noexcept
		{
			return a.m_path == b;
		}

		constexpr void swap(cfg_path &other) noexcept
		{
			using std::swap;
			swap(m_path, other.m_path);
			swap(m_slices, other.m_slices);
		}
		friend constexpr void swap(cfg_path &a, cfg_path &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] SEK_API cfg_path to_component(const slice_t *first, const slice_t *last) const;

		SEK_API void parse();

		/** String containing the full normalized path. */
		std::string m_path;
		/** Individual elements of the path. */
		std::vector<slice_t> m_slices;
	};

	/** @brief Service used to manage configuration entries.
	 *
	 * Engine configuration is stored as entries within the config registry.
	 * Every entry belongs to a category, and is created at plugin initialization time.
	 * Categories are de-serialized from individual Json files or loaded directly from Json node trees.
	 * When a new entry is added, it is de-serialized from the cached category tree. */
	class config_registry : public service<detail::config_guard>
	{
		friend detail::config_guard;

		struct entry_node;
		struct entry_value
		{
			template<typename T>
			constexpr explicit entry_value(T &&value) : data(std::forward<T>(value))
			{
				deserialze = [](serialization::json_tree &, any_ref) {

				};
				serialze = [](serialization::json_tree &, any_ref) {

				};
			}

			/* Functions used to read & write the node. */
			void (*deserialze)(serialization::json_tree &, any_ref);
			void (*serialze)(serialization::json_tree &, any_ref);

			any data; /* Deserialized data of the entry. */
		};

		struct entry_hash
		{
			typedef std::true_type is_transparent;

			hash_t operator()(std::string_view key) const noexcept { return fnv1a(key.data(), key.size()); }
			hash_t operator()(const std::string &key) const noexcept { return operator()(std::string_view{key}); }
			hash_t operator()(const entry_node *node) const noexcept { return operator()(node->path.string()); }
		};
		struct entry_cmp
		{
			typedef std::true_type is_transparent;

			hash_t operator()(const entry_node *a, std::string_view b) const noexcept { return a->path == b; }
			hash_t operator()(std::string_view a, const entry_node *b) const noexcept { return a == b->path; }
			hash_t operator()(const entry_node *a, const std::string &b) const noexcept { return a->path == b; }
			hash_t operator()(const std::string &a, const entry_node *b) const noexcept { return a == b->path; }
			bool operator()(const entry_node *a, const entry_node *b) const noexcept { return a == b; }
		};

		using entry_set = dense_set<entry_node *, entry_hash, entry_cmp>;

		struct entry_node
		{
			constexpr explicit entry_node(cfg_path &&path) noexcept : path(std::move(path)) {}

			cfg_path path;		/* Full path of the entry. */
			entry_set children; /* Immediate children of the entry (if any). */

			/* Optional value of the entry (present if the entry is initialized) */
			entry_value *value = nullptr;
		};

		template<bool IsConst>
		class entry_ref;
		template<bool IsConst>
		class entry_ptr;
		template<bool IsConst>
		class entry_iterator;

		template<bool IsConst>
		class value_ptr
		{
			template<bool>
			friend class entry_ref;

			constexpr explicit value_ptr(entry_value *ptr) noexcept : m_ptr(ptr) {}

		public:
			typedef any value_type;
			typedef std::conditional_t<IsConst, const any, any> *pointer;
			typedef std::conditional_t<IsConst, const any, any> &reference;

		public:
			/** Initializes a null value pointer. */
			constexpr value_ptr() noexcept = default;

			// clang-format off
			template<bool OtherConst = !IsConst>
			constexpr value_ptr(const value_ptr<OtherConst> &other) noexcept requires(IsConst && !OtherConst)
				: m_ptr(other.m_ptr) {}
			// clang-format on

			/** Checks if the value pointer is null. */
			[[nodiscard]] constexpr operator bool() const noexcept { return m_ptr != nullptr; }

			/** Returns pointer to the underlying 'any' value. */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return &m_ptr->data; }
			/** Returns reference to the underlying 'any' value. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return m_ptr->data; }

			[[nodiscard]] constexpr auto operator<=>(const value_ptr &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const value_ptr &) const noexcept = default;

			constexpr void swap(value_ptr &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(value_ptr &a, value_ptr &b) noexcept { a.swap(b); }

		private:
			entry_value *m_ptr = nullptr;
		};
		template<bool IsConst>
		class entry_ref
		{
			template<bool>
			friend class entry_ref;
			friend class config_registry;

			constexpr explicit entry_ref(entry_node *node) noexcept : m_node(node) {}

		public:
			typedef entry_ref value_type;
			typedef entry_iterator<IsConst> iterator;
			typedef entry_iterator<true> const_iterator;

		public:
			entry_ref() = delete;

			/** Returns pointer to self. */
			[[nodiscard]] constexpr entry_ref *operator&() noexcept { return this; }
			/** @copydoc operator& */
			[[nodiscard]] constexpr const entry_ref *operator&() const noexcept { return this; }

			/** Returns iterator to the first child of the entry. */
			[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_node->children.begin()}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator begin() const noexcept
			{
				return const_iterator{m_node->children.begin()};
			}
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last child of the entry. */
			[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_node->children.end()}; }
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator end() const noexcept
			{
				return const_iterator{m_node->children.end()};
			}
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

			/** Returns reference to the first child of the entry. */
			[[nodiscard]] constexpr entry_ref front() noexcept { return *begin(); }
			/** @copydoc front */
			[[nodiscard]] constexpr entry_ref<true> front() const noexcept { return *begin(); }
			/** Returns reference to the last child of the entry. */
			[[nodiscard]] constexpr entry_ref back() noexcept { return *(--end()); }
			/** @copydoc back */
			[[nodiscard]] constexpr entry_ref<true> back() const noexcept { return *(--end()); }

			/** Returns reference to the config path of the entry. */
			[[nodiscard]] constexpr const cfg_path &path() const noexcept { return m_node->path; }
			/** Returns pointer to the value of the entry. */
			[[nodiscard]] constexpr auto value() const noexcept { return value_ptr<IsConst>{m_node->value}; }

			[[nodiscard]] constexpr auto operator<=>(const entry_ref &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const entry_ref &) const noexcept = default;

			constexpr void swap(entry_ref &other) noexcept { std::swap(m_node, other.m_node); }
			friend constexpr void swap(entry_ref &a, entry_ref &b) noexcept { a.swap(b); }

		private:
			entry_node *m_node = nullptr;
		};
		template<bool IsConst>
		class entry_ptr
		{
			template<bool>
			friend class entry_iterator;
			friend class config_registry;

			constexpr explicit entry_ptr(entry_node *node) noexcept : m_ref(node) {}

		public:
			typedef entry_ref<IsConst> value_type;
			typedef entry_ref<IsConst> reference;
			typedef const value_type *pointer;

		public:
			/** Initializes a null entry pointer. */
			constexpr entry_ptr() noexcept = default;

			// clang-format off
			template<bool OtherConst = !IsConst>
			constexpr entry_ptr(const entry_ptr<OtherConst> &other) noexcept requires(IsConst && !OtherConst)
				: m_ref(other.m_ref) {}
			// clang-format on

			/** Checks if the entry pointer is null. */
			[[nodiscard]] constexpr operator bool() const noexcept { return m_ref.m_node != nullptr; }

			/** Returns pointer to the underlying entry. */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return &m_ref; }
			/** Returns reference to the underlying entry. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return m_ref; }

			[[nodiscard]] constexpr auto operator<=>(const entry_ptr &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const entry_ptr &) const noexcept = default;

			constexpr void swap(entry_ptr &other) noexcept { m_ref.swap(other.m_ref); }
			friend constexpr void swap(entry_ptr &a, entry_ptr &b) noexcept { a.swap(b); }

		private:
			reference m_ref = reference{nullptr};
		};
		template<bool IsConst>
		class entry_iterator
		{
			friend class config_registry;

			using iter_type = std::conditional_t<IsConst, typename entry_set::const_iterator, typename entry_set::iterator>;

			constexpr explicit entry_iterator(iter_type iter) noexcept : m_iter(iter) {}

		public:
			typedef entry_ref<IsConst> value_type;
			typedef entry_ptr<IsConst> pointer;
			typedef entry_ref<IsConst> reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		public:
			constexpr entry_iterator() noexcept = default;

			// clang-format off
			template<bool OtherConst = !IsConst>
			constexpr entry_iterator(const entry_iterator<OtherConst> &other) noexcept requires(IsConst && !OtherConst)
				: m_iter(other.m_iter) {}
			// clang-format on

			constexpr entry_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr entry_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}
			constexpr entry_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr entry_iterator &operator--() noexcept
			{
				--m_iter;
				return *this;
			}

			/** Returns pointer to the underlying entry. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{*m_iter}; }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the underlying entry. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const entry_iterator &other) const noexcept
			{
				return get() <=> other.get();
			}
			[[nodiscard]] constexpr bool operator==(const entry_iterator &other) const noexcept
			{
				return get() == other.get();
			}

			constexpr void swap(entry_iterator &other) noexcept { m_iter.swap(other.m_iter); }
			friend constexpr void swap(entry_iterator &a, entry_iterator &b) noexcept { a.swap(b); }

		private:
			iter_type m_iter;
		};

	public:
		typedef entry_iterator<false> iterator;
		typedef entry_iterator<true> const_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	public:
		/** Returns iterator to the first category of the registry. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_categories.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{m_categories.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last category of the registry. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_categories.end()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_categories.end()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		/** Returns reference to the first category of the registry. */
		[[nodiscard]] constexpr auto front() noexcept { return *begin(); }
		/** @copydoc front */
		[[nodiscard]] constexpr auto front() const noexcept { return *begin(); }
		/** Returns reference to the last category of the registry. */
		[[nodiscard]] constexpr auto back() noexcept { return *(--end()); }
		/** @copydoc back */
		[[nodiscard]] constexpr auto back() const noexcept { return *(--end()); }

		/** Returns entry pointer to the entry with the specified path. */
		[[nodiscard]] SEK_API entry_ptr<false> find(const cfg_path &path);
		/** @copydoc find */
		[[nodiscard]] SEK_API entry_ptr<true> find(const cfg_path &path) const;

		/** Creates a config entry of type `T`.
		 * @param path Full path of the entry.
		 * @return Reference to the value of the entry. */
		template<typename T>
		inline T &try_insert(cfg_path path)
		{
			if (auto existing = find(path); existing && existing->value()) [[unlikely]]
				return existing->value()->template cast<T &>();
			else
				return insert(std::move(path), T{});
		}
		/** Creates or replaces a config entry of type `T`.
		 * @param path Full path of the entry.
		 * @param value Value of the entry.
		 * @return Reference to the value of the entry.
		 * @note If such entry already exists, replaces the value. */
		template<typename T>
		inline T &insert(cfg_path path, T value = {})
		{
			if (auto existing = find(path); existing && existing->value()) [[unlikely]]
				return (*existing->value() = make_any(std::move(value))).template cast<T &>();
			else
				return insert(std::move(path), std::move(value));
		}

		/** Erases the specified config entry and all it's children.
		 * @return `true` If the entry was erased, `false` otherwise. */
		SEK_API bool erase(entry_ptr<true> where);
		/** @copydoc erase */
		SEK_API bool erase(const cfg_path &path);

	private:
		template<typename T>
		inline T &insert_impl(cfg_path &&path, T &&value)
		{
			if (path.empty()) [[unlikely]]
				throw config_error("Entry path cannot be empty");

			auto *ptr = m_value_pool.allocate();
			try
			{
				std::construct_at(ptr, std::forward<T>(value));
			}
			catch (...)
			{
				m_value_pool.deallocate(ptr);
				throw;
			}

			insert_impl(std::forward<cfg_path>(path), ptr);
			return ptr->data.template cast<T &>();
		}
		entry_node *insert_impl(cfg_path &&path);
		SEK_API void insert_impl(cfg_path &&path, entry_value *value);

		void erase_impl(typename entry_set::const_iterator where);

		detail::basic_pool<entry_value> m_value_pool; /* Pool used to allocate entry values. */
		detail::basic_pool<entry_node> m_node_pool;	  /* Pool used to allocate entry nodes. */

		entry_set m_categories; /* Categories of the registry. */
		entry_set m_entries;	/* Entry nodes of the registry. */
	};

	namespace attributes
	{
		/** @brief Attribute type used to and automatically create a config entry. */
		template<typename T>
		class config_entry
		{
		public:
			config_entry(cfg_path path)
			{
				config_registry::instance()->access_unique()->template insert<T>(std::move(path));
			}
		};
	}	 // namespace attributes
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::engine::detail::config_guard>;