//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "any.hpp"

namespace sek
{
	namespace detail
	{
		// clang-format off
		template<typename T>
		concept table_range_type = std::ranges::forward_range<T> && requires
		{
			typename T::key_type;
			typename T::mapped_type;
		};
		// clang-format on

		template<typename I>
		class table_type_iterator final : table_type_iterator<void>
		{
		public:
			constexpr table_type_iterator() noexcept = default;
			~table_type_iterator() final = default;

			constexpr table_type_iterator(const I &iter) noexcept : m_iter(iter) {}
			constexpr table_type_iterator(I &&iter) noexcept : m_iter(std::move(iter)) {}

			[[nodiscard]] table_type_iterator<void> *make_copy() const final { return new table_type_iterator{m_iter}; }

			[[nodiscard]] bool is_bidirectional() const noexcept final { return std::bidirectional_iterator<I>; }
			[[nodiscard]] bool is_random_access() const noexcept final { return std::random_access_iterator<I>; }

			void inc() final
			{
				if constexpr (std::forward_iterator<I>) --m_iter;
			}
			void inc(difference_type n) final
			{
				if constexpr (std::random_access_iterator<I>) m_iter += n;
			}
			void dec() final
			{
				if constexpr (std::bidirectional_iterator<I>) --m_iter;
			}
			void dec(difference_type n) final
			{
				if constexpr (std::random_access_iterator<I>) m_iter -= n;
			}

			[[nodiscard]] difference_type operator-(const table_type_iterator<void> &other) const final
			{
				if constexpr (std::random_access_iterator<I>)
					return m_iter - static_cast<const table_type_iterator &>(other).m_iter;
				else
					return 0;
			}

			[[nodiscard]] any value() const final { return forward_any(*m_iter); }
			[[nodiscard]] any key() const final
			{
				if constexpr (pair_like<std::iter_value_t<I>>)
					return forward_any(m_iter->first);
				else
					return value();
			}
			[[nodiscard]] any mapped() const final
			{
				if constexpr (pair_like<std::iter_value_t<I>>)
					return forward_any(m_iter->second);
				else
					return value();
			}

			[[nodiscard]] bool operator==(const table_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const table_type_iterator &>(other).m_iter;
				if (requires { m_iter == other_iter; })
					return m_iter == other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator<(const table_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const table_type_iterator &>(other).m_iter;
				if (requires { m_iter < other_iter; })
					return m_iter < other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator<=(const table_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const table_type_iterator &>(other).m_iter;
				if (requires { m_iter <= other_iter; })
					return m_iter <= other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator>(const table_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const table_type_iterator &>(other).m_iter;
				if (requires { m_iter > other_iter; })
					return m_iter > other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator>=(const table_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const table_type_iterator &>(other).m_iter;
				if (requires { m_iter >= other_iter; })
					return m_iter > other_iter;
				else
					return false;
			}

		private:
			I m_iter;
		};

		template<typename T>
		constexpr table_type_data table_type_data::make_instance() noexcept
		{
			using key_t = typename T::key_type;
			using mapped_t = typename T::mapped_type;
			using iter_t = table_type_iterator<std::ranges::iterator_t<T>>;
			using const_iter_t = table_type_iterator<std::ranges::iterator_t<const T>>;

			table_type_data result;
			result.value_type = type_handle{type_selector<std::ranges::range_value_t<T>>};
			result.key_type = type_handle{type_selector<key_t>};
			result.mapped_type = type_handle{type_selector<mapped_t>};

			result.empty = +[](const void *p) -> bool { return std::ranges::empty(*static_cast<const T *>(p)); };
			if constexpr (std::ranges::sized_range<T>)
				result.size = +[](const void *p) -> std::size_t
				{
					const auto &obj = *static_cast<const T *>(p);
					return static_cast<std::size_t>(std::ranges::size(obj));
				};
			if constexpr (requires(const T &t, const key_t &key) { t.contains(key); })
				result.contains = +[](const void *p, const any &key) -> bool
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					return static_cast<const T *>(p)->contains(key_obj);
				};

			// clang-format off
			if constexpr (requires(const T &t, const key_t &key) { t.find(key); } &&
						  requires(T &t, const key_t &key) { t.find(key); })
			{
				result.find = +[](any_ref &target, const any &key) -> std::unique_ptr<table_type_iterator<void>>
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					if (target.is_const()) [[unlikely]]
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::make_unique<const_iter_t>(obj.find(key_obj));
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						return std::make_unique<const_iter_t>(obj.find(key_obj));
					}
				};
				result.cfind = +[](const any_ref &target, any key) -> std::unique_ptr<table_type_iterator<void>>
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(obj.find(key_obj));
				};
			}
			// clang-format on
			result.at = +[](any_ref &target, const any &key) -> any
			{
				auto &key_obj = *static_cast<const key_t *>(key.data());
				if (target.is_const()) [[unlikely]]
				{
					auto &obj = *static_cast<const T *>(target.data());
					if constexpr (requires { obj.at(key_obj); })
						return forward_any(obj.at(key_obj));
					else if constexpr (requires { obj.find(key_obj); })
					{
						const auto iter = obj.find(key_obj);
						if (iter < std::ranges::end(obj)) [[likely]]
							return forward_any(*obj);
					}
				}
				else
				{
					auto &obj = *static_cast<T *>(target.data());
					if constexpr (requires { obj.at(key_obj); })
						return forward_any(obj.at(key_obj));
					else if constexpr (requires { obj.find(key_obj); })
					{
						const auto iter = obj.find(key_obj);
						if (iter < std::ranges::end(obj)) [[likely]]
							return forward_any(*obj);
					}
				}
				throw std::out_of_range("`key` is not present within the table");
			};
			result.cat = +[](const any_ref &target, any key) -> any
			{
				auto &key_obj = *static_cast<const key_t *>(key.data());
				auto &obj = *static_cast<const T *>(target.data());
				if constexpr (requires { obj.at(key_obj); })
					return forward_any(obj.at(key_obj));
				else if constexpr (requires { obj.find(key_obj); })
				{
					const auto iter = obj.find(key_obj);
					if (iter < std::ranges::end(obj)) [[likely]]
						return forward_any(*obj);
				}
				throw std::out_of_range("`key` is not present within the table");
			};

			result.begin = +[](any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
			{
				if (target.is_const()) [[unlikely]]
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::ranges::begin(obj));
				}
				else
				{
					auto &obj = *static_cast<T *>(target.data());
					return std::make_unique<iter_t>(std::ranges::begin(obj));
				}
			};
			result.cbegin = +[](const any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
			{
				auto &obj = *static_cast<const T *>(target.data());
				return std::make_unique<const_iter_t>(std::ranges::begin(obj));
			};
			result.end = +[](any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
			{
				if (target.is_const()) [[unlikely]]
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::ranges::end(obj));
				}
				else
				{
					auto &obj = *static_cast<T *>(target.data());
					return std::make_unique<iter_t>(std::ranges::end(obj));
				}
			};
			result.cend = +[](const any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
			{
				auto &obj = *static_cast<const T *>(target.data());
				return std::make_unique<const_iter_t>(std::ranges::end(obj));
			};

			if constexpr (std::ranges::bidirectional_range<T>)
			{
				result.rbegin = +[](any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
				{
					if (target.is_const()) [[unlikely]]
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::make_unique<const_iter_t>(std::prev(std::ranges::end(obj)));
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						return std::make_unique<iter_t>(std::prev(std::ranges::end(obj)));
					}
				};
				result.crbegin = +[](const any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::prev(std::ranges::end(obj)));
				};
				result.rend = +[](any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
				{
					if (target.is_const()) [[unlikely]]
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::make_unique<const_iter_t>(std::next(std::ranges::begin(obj)));
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						return std::make_unique<iter_t>(std::next(std::ranges::begin(obj)));
					}
				};
				result.crend = +[](const any_ref &target) -> std::unique_ptr<table_type_iterator<void>>
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::next(std::ranges::begin(obj)));
				};
			}

			return result;
		}
		template<typename T>
		constinit const table_type_data table_type_data::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Proxy structure used to operate on a table-like type-erased object. */
	class SEK_API any_table
	{
		friend class any;
		friend class any_ref;

	public:
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		class SEK_API table_iterator
		{
			friend class any_table;

			using iter_t = detail::table_type_iterator<void>;

		public:
			typedef any value_type;
			typedef any reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

		private:
			explicit table_iterator(std::unique_ptr<iter_t> &&ptr) noexcept : m_iter(std::move(ptr)) {}

		public:
			constexpr table_iterator() noexcept = default;

			table_iterator(const table_iterator &);
			table_iterator &operator=(const table_iterator &);
			table_iterator(table_iterator &&) noexcept = default;
			table_iterator &operator=(table_iterator &&) noexcept = default;

			/** Checks if the iterator is a bidirectional table_type_iterator. */
			[[nodiscard]] bool is_bidirectional() const noexcept;
			/** Checks if the iterator is a random access table_type_iterator. */
			[[nodiscard]] bool is_random_access() const noexcept;

			/** Post-increments the iterator if it is a bidirectional iterator, otherwise returns copy of `this`. */
			table_iterator operator++(int);
			/** Pre-increments the iterator if it is a bidirectional iterator, otherwise does nothing. */
			table_iterator &operator++();
			/** Increments the iterator by `n` if it is a random access iterator, otherwise does nothing. */
			table_iterator &operator+=(difference_type n);
			/** Post-decrements the iterator if it is a bidirectional iterator, otherwise returns copy of `this`. */
			table_iterator operator--(int);
			/** Pre-decrements the iterator if it is a bidirectional iterator, otherwise does nothing. */
			table_iterator &operator--();
			/** Decrements the iterator by `n` if it is a random access iterator, otherwise does nothing. */
			table_iterator &operator-=(difference_type n);

			/** Returns a copy of this iterator incremented by `n` if it is a random access iterator, otherwise returns copy of `this`. */
			[[nodiscard]] table_iterator operator+(difference_type n) const;
			/** Returns a copy of this iterator decremented by `n` if it is a random access iterator, otherwise returns copy of `this`. */
			[[nodiscard]] table_iterator operator-(difference_type n) const;
			/** Returns difference between `this` and `other`, if `this` is a random access iterator, otherwise returns 0. */
			[[nodiscard]] difference_type operator-(const table_iterator &other) const;

			/** Returns the object pointed to by the iterator. */
			[[nodiscard]] any value() const;
			/** If the iterator points to a key-mapped pair, returns the key object. Otherwise, returns the pointed-to object. */
			[[nodiscard]] any key() const;
			/** If the iterator points to a key-mapped pair, returns the mapped object. Otherwise, returns the pointed-to object. */
			[[nodiscard]] any mapped() const;
			/** @copydoc value */
			[[nodiscard]] any operator*() const { return value(); }
			/** Returns the object located at an offset from this iterator if it is a random access iterator, otherwise returns empty `any`. */
			[[nodiscard]] any operator[](difference_type n) const;

			[[nodiscard]] bool operator==(const table_iterator &) const;
			[[nodiscard]] bool operator>(const table_iterator &) const;
			[[nodiscard]] bool operator>=(const table_iterator &) const;
			[[nodiscard]] bool operator<(const table_iterator &) const;
			[[nodiscard]] bool operator<=(const table_iterator &) const;

		private:
			std::unique_ptr<iter_t> m_iter;
		};

	public:
		typedef table_iterator iterator;
		typedef table_iterator const_iterator;
		typedef std::reverse_iterator<table_iterator> reverse_iterator;
		typedef std::reverse_iterator<table_iterator> const_reverse_iterator;
		typedef typename table_iterator::reference reference;
		typedef typename table_iterator::reference const_reference;

	private:
		any_table(std::in_place_t, const any_ref &ref) : m_data(ref.m_type->table_data), m_target(ref) {}
		any_table(std::in_place_t, any_ref &&ref) : m_data(ref.m_type->table_data), m_target(std::move(ref)) {}

		static const detail::table_type_data *assert_data(const detail::type_data *data);

	public:
		any_table() = delete;
		any_table(const any_table &) = delete;
		any_table &operator=(const any_table &) = delete;

		constexpr any_table(any_table &&other) noexcept : m_data(other.m_data), m_target(std::move(other.m_target)) {}
		constexpr any_table &operator=(any_table &&other) noexcept
		{
			m_data = other.m_data;
			m_target = std::move(other.m_target);
			return *this;
		}

		/** Initializes an `any_table` instance for an `any_ref` object.
		 * @param ref `any_ref` referencing a table object.
		 * @throw type_error If the referenced object is not a table. */
		explicit any_table(const any_ref &ref) : m_data(assert_data(ref.m_type)), m_target(ref) {}
		/** @copydoc any_table */
		explicit any_table(any_ref &&ref) : m_data(assert_data(ref.m_type)), m_target(std::move(ref)) {}

		/** Returns `any_ref` reference ot the target table. */
		[[nodiscard]] any_ref target() const noexcept { return m_target; }

		/** Checks if the referenced table is a sized range. */
		[[nodiscard]] constexpr bool is_sized_range() const noexcept { return m_data->size != nullptr; }
		/** Checks if the referenced table is a bidirectional range. */
		[[nodiscard]] constexpr bool is_bidirectional_range() const noexcept { return m_data->rbegin != nullptr; }
		/** Checks if the referenced table is a random access range. */
		[[nodiscard]] constexpr bool is_random_access_range() const noexcept { return m_data->at != nullptr; }

		/** Returns the value type of the table. */
		[[nodiscard]] constexpr type_info value_type() const noexcept;
		/** Returns the key type of the table. */
		[[nodiscard]] constexpr type_info key_type() const noexcept;
		/** Returns the mapped type of the table. */
		[[nodiscard]] constexpr type_info mapped_type() const noexcept;

		/** Returns iterator to the first element of the referenced table. */
		[[nodiscard]] iterator begin();
		/** @copydoc begin */
		[[nodiscard]] const_iterator begin() const;
		/** @copydoc begin */
		[[nodiscard]] const_iterator cbegin() const { return begin(); }
		/** Returns iterator one past to the last element of the referenced table. */
		[[nodiscard]] iterator end();
		/** @copydoc end */
		[[nodiscard]] const_iterator end() const;
		/** @copydoc end */
		[[nodiscard]] const_iterator cend() const { return end(); }
		/** Returns reverse iterator to the last element of the referenced table,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rbegin();
		/** @copydoc rbegin */
		[[nodiscard]] const_reverse_iterator rbegin() const;
		/** @copydoc rbegin */
		[[nodiscard]] const_reverse_iterator crbegin() const { return rbegin(); }
		/** Returns reverse iterator one past the first element of the referenced table,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rend();
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator rend() const;
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator crend() const { return rend(); }

		/** Checks if the referenced table contains the specified key. */
		[[nodiscard]] bool contains(const any &key) const;
		/** Returns iterator to the element of the referenced table located at the specified key, or the end iterator. */
		[[nodiscard]] iterator find(const any &key);
		/** @copydoc begin */
		[[nodiscard]] const_iterator find(const any &key) const;

		/** Checks if the referenced table is empty. */
		[[nodiscard]] bool empty() const;
		/** Returns size of the referenced table, or `0` if the table is not a sized range. */
		[[nodiscard]] size_type size() const;

		/** Returns the element of the referenced table located at the specified key.
		 * @throw std::out_of_range If the key is not present within the table. */
		[[nodiscard]] any at(const any &key);
		/** @copydoc at */
		[[nodiscard]] any operator[](const any &key) { return at(key); }
		/** @copydoc at */
		[[nodiscard]] any at(const any &key) const;
		/** @copydoc at */
		[[nodiscard]] any operator[](const any &key) const { return at(key); }

		constexpr void swap(any_table &other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_target, other.m_target);
		}
		friend constexpr void swap(any_table &a, any_table &b) noexcept { a.swap(b); }

	private:
		const detail::table_type_data *m_data;
		any_ref m_target;
	};
}	 // namespace sek