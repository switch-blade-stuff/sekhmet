//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "any.hpp"
#include "type_info.hpp"

namespace sek
{
	namespace detail
	{
		template<typename I>
		class range_type_iterator final : range_type_iterator<void>
		{
		public:
			constexpr range_type_iterator() noexcept = default;
			~range_type_iterator() final = default;

			constexpr range_type_iterator(const I &iter) noexcept : m_iter(iter) {}
			constexpr range_type_iterator(I &&iter) noexcept : m_iter(std::move(iter)) {}

			[[nodiscard]] range_type_iterator<void> *make_copy() const final { return new range_type_iterator{m_iter}; }

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

			[[nodiscard]] difference_type operator-(const range_type_iterator<void> &other) const final
			{
				if constexpr (std::random_access_iterator<I>)
					return m_iter - static_cast<const range_type_iterator &>(other).m_iter;
				else
					return 0;
			}

			[[nodiscard]] any value() const final { return forward_any(*m_iter); }

			[[nodiscard]] bool operator==(const range_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const range_type_iterator &>(other).m_iter;
				if (requires { m_iter == other_iter; })
					return m_iter == other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator<(const range_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const range_type_iterator &>(other).m_iter;
				if (requires { m_iter < other_iter; })
					return m_iter < other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator<=(const range_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const range_type_iterator &>(other).m_iter;
				if (requires { m_iter <= other_iter; })
					return m_iter <= other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator>(const range_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const range_type_iterator &>(other).m_iter;
				if (requires { m_iter > other_iter; })
					return m_iter > other_iter;
				else
					return false;
			}
			[[nodiscard]] bool operator>=(const range_type_iterator<void> &other) const noexcept final
			{
				const auto &other_iter = static_cast<const range_type_iterator &>(other).m_iter;
				if (requires { m_iter >= other_iter; })
					return m_iter > other_iter;
				else
					return false;
			}

		private:
			I m_iter;
		};

		template<typename T>
		constexpr range_type_data range_type_data::make_instance() noexcept
		{
			range_type_data result;
			result.value_type = type_handle{type_selector<std::ranges::range_value_t<T>>};

			result.empty = +[](const any_ref &target) -> bool
			{
				auto &obj = *static_cast<const T *>(target.data());
				return std::ranges::empty(obj);
			};
			if constexpr (std::ranges::sized_range<T>)
				result.size = +[](const any_ref &target) -> std::size_t
				{
					auto &obj = *static_cast<const T *>(target.data());
					return static_cast<std::size_t>(std::ranges::size(obj));
				};

			if constexpr (std::ranges::forward_range<T>)
			{
				using iter_t = range_type_iterator<std::ranges::iterator_t<T>>;
				using const_iter_t = range_type_iterator<std::ranges::iterator_t<const T>>;

				result.begin = +[](any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
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
				result.cbegin = +[](const any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::ranges::begin(obj));
				};
				result.end = +[](any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
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
				result.cend = +[](const any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::ranges::end(obj));
				};

				result.front = +[](any_ref &target) -> any
				{
					if (target.is_const()) [[unlikely]]
					{
						auto &obj = *static_cast<const T *>(target.data());
						return forward_any(*std::ranges::begin(obj));
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						return forward_any(*std::ranges::begin(obj));
					}
				};
				result.cfront = +[](const any_ref &target) -> any
				{
					auto &obj = *static_cast<const T *>(target.data());
					return forward_any(*std::ranges::begin(obj));
				};
			}
			if constexpr (std::ranges::bidirectional_range<T>)
			{
				using iter_t = range_type_iterator<std::ranges::iterator_t<T>>;
				using const_iter_t = range_type_iterator<std::ranges::iterator_t<const T>>;

				result.rbegin = +[](any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
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
				result.crbegin = +[](const any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::prev(std::ranges::end(obj)));
				};
				result.rend = +[](any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
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
				result.crend = +[](const any_ref &target) -> std::unique_ptr<range_type_iterator<void>>
				{
					auto &obj = *static_cast<const T *>(target.data());
					return std::make_unique<const_iter_t>(std::next(std::ranges::begin(obj)));
				};

				result.back = +[](any_ref &target) -> any
				{
					if (target.is_const()) [[unlikely]]
					{
						auto &obj = *static_cast<const T *>(target.data());
						return forward_any(*std::prev(std::ranges::end(obj)));
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						return forward_any(*std::prev(std::ranges::end(obj)));
					}
				};
				result.cback = +[](const any_ref &target) -> any
				{
					auto &obj = *static_cast<const T *>(target.data());
					return forward_any(*std::prev(std::ranges::end(obj)));
				};
			}
			if constexpr (std::ranges::random_access_range<T>)
			{
				using diff_type = std::ranges::range_difference_t<T>;
				result.at = +[](any_ref &target, std::size_t i) -> any
				{
					if (target.is_const()) [[unlikely]]
					{
						auto &obj = *static_cast<const T *>(target.data());
						if (i >= std::ranges::size(obj)) [[unlikely]]
							throw std::out_of_range("`i` is out of bounds");

						return forward_any(std::ranges::begin(obj)[static_cast<diff_type>(i)]);
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						if (i >= std::ranges::size(obj)) [[unlikely]]
							throw std::out_of_range("`i` is out of bounds");

						return forward_any(std::ranges::begin(obj)[static_cast<diff_type>(i)]);
					}
				};
				result.cat = +[](const any_ref &target, std::size_t i) -> any
				{
					auto &obj = *static_cast<const T *>(target.data());
					if (i >= std::ranges::size(obj)) [[unlikely]]
						throw std::out_of_range("`i` is out of bounds");

					return forward_any(std::ranges::begin(obj)[static_cast<diff_type>(i)]);
				};
			}

			return result;
		}
		template<typename T>
		constinit const range_type_data range_type_data::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Proxy structure used to operate on a range-like type-erased object. */
	class SEK_API any_range
	{
		friend class any;
		friend class any_ref;

	public:
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		class SEK_API range_iterator
		{
			friend class any_range;

			using iter_t = detail::range_type_iterator<void>;

		public:
			typedef any value_type;
			typedef any reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

		private:
			explicit range_iterator(std::unique_ptr<iter_t> &&ptr) noexcept : m_iter(std::move(ptr)) {}

		public:
			constexpr range_iterator() noexcept = default;

			range_iterator(const range_iterator &);
			range_iterator &operator=(const range_iterator &);
			range_iterator(range_iterator &&) noexcept = default;
			range_iterator &operator=(range_iterator &&) noexcept = default;

			/** Checks if the iterator is a bidirectional range_type_iterator. */
			[[nodiscard]] bool is_bidirectional() const noexcept;
			/** Checks if the iterator is a random access range_type_iterator. */
			[[nodiscard]] bool is_random_access() const noexcept;

			/** Post-increments the iterator if it is a bidirectional iterator, otherwise returns copy of `this`. */
			range_iterator operator++(int);
			/** Pre-increments the iterator if it is a bidirectional iterator, otherwise does nothing. */
			range_iterator &operator++();
			/** Increments the iterator by `n` if it is a random access iterator, otherwise does nothing. */
			range_iterator &operator+=(difference_type n);
			/** Post-decrements the iterator if it is a bidirectional iterator, otherwise returns copy of `this`. */
			range_iterator operator--(int);
			/** Pre-decrements the iterator if it is a bidirectional iterator, otherwise does nothing. */
			range_iterator &operator--();
			/** Decrements the iterator by `n` if it is a random access iterator, otherwise does nothing. */
			range_iterator &operator-=(difference_type n);

			/** Returns a copy of this iterator incremented by `n` if it is a random access iterator, otherwise returns copy of `this`. */
			[[nodiscard]] range_iterator operator+(difference_type n) const;
			/** Returns a copy of this iterator decremented by `n` if it is a random access iterator, otherwise returns copy of `this`. */
			[[nodiscard]] range_iterator operator-(difference_type n) const;
			/** Returns difference between `this` and `other`, if `this` is a random access iterator, otherwise returns 0. */
			[[nodiscard]] difference_type operator-(const range_iterator &other) const;

			/** Returns the object pointed to by the iterator. */
			[[nodiscard]] any value() const;
			/** @copydoc value */
			[[nodiscard]] any operator*() const { return value(); }
			/** Returns the object located at an offset from this iterator if it is a random access iterator, otherwise returns empty `any`. */
			[[nodiscard]] any operator[](difference_type n) const;

			[[nodiscard]] bool operator==(const range_iterator &) const;
			[[nodiscard]] bool operator>(const range_iterator &) const;
			[[nodiscard]] bool operator>=(const range_iterator &) const;
			[[nodiscard]] bool operator<(const range_iterator &) const;
			[[nodiscard]] bool operator<=(const range_iterator &) const;

		private:
			std::unique_ptr<iter_t> m_iter;
		};

	public:
		typedef range_iterator iterator;
		typedef range_iterator const_iterator;
		typedef std::reverse_iterator<range_iterator> reverse_iterator;
		typedef std::reverse_iterator<range_iterator> const_reverse_iterator;
		typedef typename range_iterator::reference reference;
		typedef typename range_iterator::reference const_reference;

	private:
		any_range(std::in_place_t, const any_ref &ref) : m_data(ref.m_type->range_data), m_target(ref) {}
		any_range(std::in_place_t, any_ref &&ref) : m_data(ref.m_type->range_data), m_target(std::move(ref)) {}

		static const detail::range_type_data *assert_data(const detail::type_data *data);

	public:
		any_range() = delete;
		any_range(const any_range &) = delete;
		any_range &operator=(const any_range &) = delete;

		constexpr any_range(any_range &&other) noexcept : m_data(other.m_data), m_target(std::move(other.m_target)) {}
		constexpr any_range &operator=(any_range &&other) noexcept
		{
			m_data = other.m_data;
			m_target = std::move(other.m_target);
			return *this;
		}

		/** Initializes an `any_range` instance for an `any_ref` object.
		 * @param ref `any_ref` referencing a range object.
		 * @throw type_error If the referenced object is not a range. */
		explicit any_range(const any_ref &ref) : m_data(assert_data(ref.m_type)), m_target(ref) {}
		/** @copydoc any_range */
		explicit any_range(any_ref &&ref) : m_data(assert_data(ref.m_type)), m_target(std::move(ref)) {}

		/** Checks if the referenced range is a sized range. */
		[[nodiscard]] constexpr bool is_sized_range() const noexcept { return m_data->size != nullptr; }
		/** Checks if the referenced range is a forward range. */
		[[nodiscard]] constexpr bool is_forward_range() const noexcept { return m_data->begin != nullptr; }
		/** Checks if the referenced range is a bidirectional range. */
		[[nodiscard]] constexpr bool is_bidirectional_range() const noexcept { return m_data->rbegin != nullptr; }
		/** Checks if the referenced range is a random access range. */
		[[nodiscard]] constexpr bool is_random_access_range() const noexcept { return m_data->at != nullptr; }

		/** Returns the value type of the range. */
		[[nodiscard]] constexpr type_info value_type() const noexcept;

		/** Returns iterator to the first element of the referenced range,
		 * or a default-constructed sentinel if it is not a forward range. */
		[[nodiscard]] iterator begin();
		/** @copydoc begin */
		[[nodiscard]] const_iterator begin() const;
		/** @copydoc begin */
		[[nodiscard]] const_iterator cbegin() const { return begin(); }
		/** Returns iterator one past to the last element of the referenced range,
		 * or a default-constructed sentinel if it is not a forward range. */
		[[nodiscard]] iterator end();
		/** @copydoc end */
		[[nodiscard]] const_iterator end() const;
		/** @copydoc end */
		[[nodiscard]] const_iterator cend() const { return end(); }
		/** Returns reverse iterator to the last element of the referenced range,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rbegin();
		/** @copydoc rbegin */
		[[nodiscard]] const_reverse_iterator rbegin() const;
		/** @copydoc rbegin */
		[[nodiscard]] const_reverse_iterator crbegin() const { return rbegin(); }
		/** Returns reverse iterator one past the first element of the referenced range,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rend();
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator rend() const;
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator crend() const { return rend(); }

		/** Checks if the referenced range is empty. */
		[[nodiscard]] bool empty() const;
		/** Returns size of the referenced range, or `0` if the range is not a sized range. */
		[[nodiscard]] size_type size() const;

		/** Returns the first object of the referenced range, or empty `any` if the range is not a forward range. */
		[[nodiscard]] any front();
		/** @copydoc front */
		[[nodiscard]] any front() const;
		/** Returns the last object of the referenced range, or empty `any` if the range is not a bidirectional range. */
		[[nodiscard]] any back();
		/** @copydoc back */
		[[nodiscard]] any back() const;
		/** Returns the `i`th object of the referenced range, or empty `any` if the range is not a random access range.
		 * @throw std::out_of_range If `i` is out of bounds of the range. */
		[[nodiscard]] any at(size_type i);
		/** @copydoc at */
		[[nodiscard]] any operator[](size_type i) { return at(i); }
		/** @copydoc at */
		[[nodiscard]] any at(size_type i) const;
		/** @copydoc at */
		[[nodiscard]] any operator[](size_type i) const { return at(i); }

		constexpr void swap(any_range &other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_target, other.m_target);
		}
		friend constexpr void swap(any_range &a, any_range &b) noexcept { a.swap(b); }

	private:
		const detail::range_type_data *m_data;
		any_ref m_target;
	};
}	 // namespace sek