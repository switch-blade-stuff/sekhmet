/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <string>

#include "sekhmet/access_guard.hpp"
#include "sekhmet/dense_set.hpp"
#include "sekhmet/detail/basic_pool.hpp"
#include "sekhmet/serialization/json.hpp"
#include "sekhmet/service.hpp"
#include "sekhmet/system/native_file.hpp"

#include "type_info.hpp"
#include <shared_mutex>

namespace sek::engine
{
	class config_registry;

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

		/** Initializes a config path from a string view. */
		cfg_path(std::string_view str) : m_path(str) { parse(); }
		/** @copydoc cfg_path */
		cfg_path(const std::string &str) : m_path(str) { parse(); }
		/** Initializes a config path from a string. */
		cfg_path(std::string &&str) : m_path(std::move(str)) { parse(); }

		/** Initializes a config path from a C-style string. */
		cfg_path(const char *str) : cfg_path(std::string_view{str}) {}
		/** Initializes a config path from a character array. */
		cfg_path(const char *str, std::size_t n) : cfg_path(std::string_view{str, n}) {}

		/** Returns the amount of elements (entry names) within the path. */
		[[nodiscard]] constexpr std::size_t elements() const noexcept { return m_slices.size(); }
		/** Checks if the config path is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return elements() == 0; }
		/** Checks if the config path is a category path. */
		[[nodiscard]] constexpr bool is_category() const noexcept { return elements() == 1; }

		/** Returns reference to the underlying path string. */
		[[nodiscard]] constexpr auto &string() noexcept { return m_path; }
		/** @copydoc string */
		[[nodiscard]] constexpr auto &string() const noexcept { return m_path; }

		/** Converts the path to a string view. */
		[[nodiscard]] constexpr operator std::string_view() noexcept { return {m_path}; }

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
		 * @return Reference to this path. */
		cfg_path &append(const cfg_path &path)
		{
			m_path.append(path.m_path);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &operator/=(const cfg_path &path) { return append(path); }
		/** Returns a path produced from concatenating two paths.
		 * @return Concatenated path. */
		[[nodiscard]] cfg_path operator/(const cfg_path &path) const
		{
			auto tmp = *this;
			tmp /= path;
			return tmp;
		}

		/** Appends a string to the path.
		 * @return Reference to this path. */
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
		 * @return Concatenated path. */
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

		[[nodiscard]] friend constexpr auto operator<=>(const char *a, const cfg_path &b) noexcept
		{
			return std::string_view{a} <=> b.m_path;
		}
		[[nodiscard]] friend constexpr bool operator==(const char *a, const cfg_path &b) noexcept
		{
			return std::string_view{a} == b.m_path;
		}
		[[nodiscard]] friend constexpr auto operator<=>(const cfg_path &a, const char *b) noexcept
		{
			return a.m_path <=> std::string_view{b};
		}
		[[nodiscard]] friend constexpr bool operator==(const cfg_path &a, const char *b) noexcept
		{
			return a.m_path == std::string_view{b};
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

	namespace attributes
	{
		class config_type;
	}

	/** @brief Service used to manage configuration entries.
	 *
	 * Engine configuration is stored as entries within the config registry.
	 * Every entry belongs to a category, and is created at plugin initialization time.
	 * Categories are de-serialized from individual Json files or loaded directly from Json node trees.
	 * When a new entry is added, it is de-serialized from the cached category tree. */
	class config_registry : public service<shared_guard<config_registry>>
	{
		friend attributes::config_type;
		friend shared_guard<config_registry>;

		struct entry_node;

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

		// clang-format off
		using output_archive = serialization::json::basic_output_archive<serialization::json::inline_arrays |
																		 serialization::json::extended_fp |
																		 serialization::json::pretty_print>;
		using input_archive = serialization::json::basic_input_archive<serialization::json::allow_comments |
																	   serialization::json::extended_fp>;
		// clang-format on
		using output_frame = typename output_archive::archive_frame;
		using input_frame = typename input_archive::archive_frame;

		struct entry_node
		{
			constexpr explicit entry_node(cfg_path &&path) noexcept : path(std::move(path)) {}
			~entry_node()
			{
				for (auto *child : nodes) std::destroy_at(child);
				delete data_cache;
			}

			struct nodes_proxy;
			struct any_proxy;

			SEK_API void serialize(output_frame &, const config_registry &) const;
			SEK_API void deserialize(input_frame &, const config_registry &);
			void deserialize(input_frame &, std::vector<entry_node *> &, const config_registry &);

			cfg_path path;	 /* Full path of the entry. */
			entry_set nodes; /* Immediate children of the entry (if any). */

			/* Optional value of the entry (present if the entry is initialized). */
			any value = {};

			/* Optional cached Json tree of the entry. */
			serialization::json_tree *data_cache = nullptr;
		};

		template<bool IsConst>
		class entry_ref;
		template<bool IsConst>
		class entry_ptr;
		template<bool IsConst>
		class entry_iterator;

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

		private:
			typedef std::conditional_t<IsConst, const any, any> any_type;

		public:
			entry_ref() = delete;

			/** Returns pointer to self. */
			[[nodiscard]] constexpr entry_ref *operator&() noexcept { return this; }
			/** @copydoc operator& */
			[[nodiscard]] constexpr const entry_ref *operator&() const noexcept { return this; }

			/** Returns iterator to the first child of the entry. */
			[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_node->nodes.begin()}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator begin() const noexcept
			{
				return const_iterator{m_node->nodes.begin()};
			}
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last child of the entry. */
			[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_node->nodes.end()}; }
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_node->nodes.end()}; }
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
			/** Returns reference to the value of the entry. */
			[[nodiscard]] constexpr any_type &value() const noexcept { return m_node->value; }

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

			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr entry_ptr(const entry_ptr<OtherConst> &other) noexcept : m_ref(other.m_ref)
			{
			}

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

			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr entry_iterator(const entry_iterator<OtherConst> &other) noexcept : m_iter(other.m_iter)
			{
			}

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
		constexpr config_registry() noexcept = default;
		SEK_API ~config_registry();

		/** Returns entry iterator to the first category of the registry. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_categories.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{m_categories.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns entry iterator one past the last category of the registry. */
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

		/** Erases all entries of the registry. */
		SEK_API void clear();

		/** Returns entry pointer to the entry with the specified path. */
		[[nodiscard]] SEK_API entry_ptr<false> find(const cfg_path &entry);
		/** @copydoc find */
		[[nodiscard]] SEK_API entry_ptr<true> find(const cfg_path &entry) const;

		/** Creates a config entry of type `T`. If needed, creates empty entries for parents of the branch.
		 * @param entry Full path of the entry.
		 * @param value Value of the entry.
		 * @return Reference to the value of the entry.
		 * @note If a new entry was inserted and there is a Json data cache up the tree, the entry will be deserialized.
		 * @throw config_error If any entry within the resulting branch fails to initialize. */
		template<typename T>
		inline T &try_insert(cfg_path entry, T value = {})
		{
			if (auto existing = find(entry); !existing) [[likely]]
				return insert_impl(std::move(entry), std::move(value));
			else
				return existing->value().template cast<T &>();
		}
		/** Creates or replaces a config entry of type `T`. If needed, creates empty entries for parents of the branch.
		 * @param entry Full path of the entry.
		 * @param value Value of the entry.
		 * @return Reference to the value of the entry.
		 * @note If such entry already exists, value of the entry will be replaced.
		 * @note If there is a Json data cache up the tree, the entry will be deserialized.
		 * @throw config_error If any entry within the resulting branch fails to initialize. */
		template<typename T>
		inline T &insert(cfg_path entry, T value = {})
		{
			if (auto existing = find(entry); !existing) [[likely]]
				return insert_impl(std::move(entry), std::move(value));
			else
				return assign_impl(existing, std::move(value));
		}

		/** Erases the specified config entry and all it's children.
		 * @return `true` If the entry was erased, `false` otherwise. */
		SEK_API bool erase(entry_ptr<true> which);
		/** @copydoc erase */
		inline bool erase(const_iterator which) { return erase(which.operator->()); }
		/** @copydoc erase */
		SEK_API bool erase(const cfg_path &entry);

		/** Loads an entry and all it's children from a Json node tree.
		 * @param entry Full path of the entry.
		 * @param tree Json node tree containing source data.
		 * @param cache If set to true, the node tree will be cached and re-used for de-serialization of new entries.
		 * @return Entry pointer to the loaded entry.
		 * @throw config_error If any entry within the resulting branch fails to initialize. */
		SEK_API entry_ptr<false> load(cfg_path entry, serialization::json_tree &&tree, bool cache = true);
		/** Loads an entry and all it's children from a Json file.
		 * @param entry Full path of the entry.
		 * @param path Path to a Json file containing source data.
		 * @param cache If set to true, the data will be cached and re-used for de-serialization of new entries.
		 * @return Entry pointer to the loaded entry.
		 * @throw config_error If the file fails to open in read mode or any entry within
		 * the resulting branch fails to initialize. */
		SEK_API entry_ptr<false> load(cfg_path entry, const std::filesystem::path &path, bool cache = true);

		/** Saves an entry and all it's children to a Json node tree.
		 * @param which Pointer to the entry to be saved.
		 * @param tree Json node tree to store the serialized data in.
		 * @return `true` on success, `false` on failure (entry does not exist). */
		SEK_API bool save(entry_ptr<true> which, serialization::json_tree &tree) const;
		/** Saves an entry and all it's children to a Json node tree.
		 * @param entry Full path of the entry.
		 * @param path Path to a the file to write Json data to.
		 * @return `true` on success, `false` on failure (entry does not exist).
		 * @throw config_error If the fails to open. */
		SEK_API bool save(entry_ptr<true> which, const std::filesystem::path &path) const;
		/** Loads an entry to a Json node tree.
		 * @param entry Full path of the entry.
		 * @param tree Json node tree to store the serialized data in.
		 * @return `true` on success, `false` on failure (entry does not exist). */
		inline bool save(const cfg_path &entry, serialization::json_tree &tree) const
		{
			return save(find(entry), tree);
		}
		/** Loads an entry from a Json file.
		 * @param entry Full path of the entry.
		 * @param path Path to a the file to write Json data to.
		 * @return `true` on success, `false` on failure (entry does not exist).
		 * @throw config_error If the fails to open. */
		inline bool save(const cfg_path &entry, const std::filesystem::path &path) const
		{
			return save(find(entry), path);
		}

	private:
		bool save_impl(entry_ptr<true> which, output_archive &archive) const;

		SEK_API entry_node *assign_impl(entry_node *node, any &&value);
		SEK_API entry_node *insert_impl(cfg_path &&entry, any &&value);
		entry_node *insert_impl(cfg_path &&entry);

		entry_node *init_branch(entry_node *node, serialization::json_tree *cache);

		template<typename T>
		inline T &insert_impl(cfg_path &&entry, T &&value)
		{
			return insert_impl(std::forward<cfg_path>(entry), make_any<T>(value))->value.template cast<T &>();
		}
		template<typename T>
		inline T &assign_impl(entry_ptr<false> ptr, T &&value)
		{
			return assign_impl(ptr.m_ref.m_node, make_any<T>(value))->value.template cast<T &>();
		}

		void erase_impl(typename entry_set::const_iterator where);
		void clear_impl();

		sek::detail::basic_pool<entry_node> m_node_pool; /* Pool used to allocate entry nodes. */

		entry_set m_categories; /* Categories of the registry. */
		entry_set m_entries;	/* Entry nodes of the registry. */
	};

	namespace attributes
	{
		/** @brief Attribute used to designate a type as a config entry and optionally auto-initialize the entry. */
		class config_type
		{
			friend class engine::config_registry;

			using output_frame = typename config_registry::output_frame;
			using input_frame = typename config_registry::input_frame;

		public:
			template<typename T>
			constexpr explicit config_type(type_selector_t<T>) noexcept
			{
				serialize = [](const any &a, output_frame &frame, const config_registry &reg)
				{
					auto &value = a.template cast<const T &>();
					if constexpr (requires { value.serialize(frame, reg); })
						value.serialize(frame, reg);
					else
					{
						using sek::serialization::serialize;
						serialize(value, frame, reg);
					}
				};
				deserialize = [](any &a, input_frame &frame, const config_registry &reg)
				{
					auto &value = a.template cast<T &>();
					if constexpr (requires { value.deserialize(frame, reg); })
						value.deserialize(frame, reg);
					else
					{
						using sek::serialization::deserialize;
						deserialize(value, frame, reg);
					}
				};
			}
			template<typename T>
			config_type(type_selector_t<T> s, cfg_path path) : config_type(s)
			{
				config_registry::instance()->access_unique()->template insert<T>(std::move(path));
			}

		private:
			void (*serialize)(const any &, output_frame &, const config_registry &);
			void (*deserialize)(any &, input_frame &, const config_registry &);
		};

		/** Helper function used to create an instance of `config_type` attribute for type `T`. */
		template<typename T>
		[[nodiscard]] constexpr config_type make_config_type() noexcept
		{
			return config_type{type_selector<T>};
		}
		/** @copydoc make_config_type
		 * @param path Config path to create an entry for. */
		template<typename T>
		[[nodiscard]] inline config_type make_config_type(cfg_path path) noexcept
		{
			return config_type{type_selector<T>, path};
		}
	}	 // namespace attributes
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::shared_guard<sek::engine::config_registry>>;