//
// Created by switchblade on 2022-02-04.
//

#pragma once

#include <cstdarg>
#include <ranges>
#include <vector>

#include "assert.hpp"
#include "engine_exception.hpp"
#include "type_data.hpp"

namespace sek
{
	class any;

	/** @brief Exception thrown when a type is invalid or incompatible with another. */
	class bad_type_exception : engine_exception
	{
	public:
		SEK_API bad_type_exception() noexcept = default;
		SEK_API explicit bad_type_exception(const char *msg) noexcept;
		SEK_API explicit bad_type_exception(type_id type) noexcept;
		SEK_API ~bad_type_exception() override;

		[[nodiscard]] const char *what() const noexcept override { return msg; }

	private:
		const char *msg = nullptr;
	};

	/** @brief Structure used to represent information about a type. */
	class type_info
	{
		using data_t = detail::type_data;

	public:
		/** Adds a type to runtime lookup database.
		 * @param type Type info of the type to add to runtime database.
		 * @return true if a type was added successfully, false otherwise.
		 * @note This function will fail if a type with the same id was already registered. */
		static SEK_API bool register_type(type_info type);
		/** Invokes type factory for the type and adds it to runtime lookup database.
		 * @return true if a type was added successfully, false otherwise.
		 * @note This function will fail if a type with the same id was already registered. */
		template<typename T>
		static bool register_type()
		{
			return register_type(get<T>());
		}
		/** Removes a type from runtime lookup database.
		 * @param type Type info of the type to remove.
		 * @return true if a type was removed successfully, false otherwise. */
		static SEK_API bool deregister_type(type_info type);
		/** Removes a type from runtime lookup database.
		 * @return true if a type was removed successfully, false otherwise. */
		template<typename T>
		static bool deregister_type()
		{
			return deregister_type(get<T>());
		}

		/** @brief RAII structure used to automatically register & deregister types to/from the runtime lookup database. */
		template<typename T>
		struct type_guard
		{
			type_guard(const type_guard &) = delete;
			type_guard &operator=(const type_guard &) = delete;
			type_guard(type_guard &&) = delete;
			type_guard &operator=(type_guard &&) = delete;

			type_guard() : added(register_type<T>()) {}
			~type_guard()
			{
				if (added) deregister_type<T>();
			}

			bool added;
		};

		/** Returns an instance of type info generated at compile time.
		 * @note Type is not required to be registered, */
		template<typename T>
		[[nodiscard]] constexpr static type_info get() noexcept
		{
			return type_info{data_t::make_handle<T>()};
		}
		/** Looks up a type within the runtime lookup database.
		 * @tparam tid Id of the type to search for.
		 * @return `type_info` instance for the requested type. If an invalid tid was specified, returns an invalid type info.
		 * @note Type must be registered for it to be available. */
		[[nodiscard]] static SEK_API type_info get(type_id tid) noexcept;
		/** Returns vector containing all currently registered types. */
		[[nodiscard]] static SEK_API std::vector<type_info> all();

	private:
		template<std::forward_iterator Iter>
		requires std::same_as<std::iter_value_t<Iter>, data_t>
		struct type_info_iterator
		{
			type_info_iterator() = delete;
			constexpr explicit type_info_iterator(Iter value) : value(value) {}

			constexpr type_info_iterator operator++(int) noexcept { return type_info_iterator{value++}; }
			constexpr type_info_iterator &operator++() noexcept
			{
				++value;
				return *this;
			}

			[[nodiscard]] constexpr type_info operator*() const noexcept { return type_info{*value}; }
			[[nodiscard]] constexpr bool operator==(const type_info_iterator &) const noexcept = default;

			Iter value;
		};

		class parent_iterator;
		class constructor_iterator;
		class attribute_iterator;

	public:
		class constructor_info;

		/** @brief Structure containing information about a function's signature (stores the return type & acts as a view for argument types). */
		class signature_info
		{
			friend class constructor_info;

		public:
			typedef type_info value_type;
			typedef type_info reference;
			typedef type_info const_reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

		private:
			class arg_type_iterator
			{
				friend class signature_info;

			public:
				typedef type_info value_type;
				typedef type_info reference;
				typedef std::ptrdiff_t difference_type;
				typedef std::contiguous_iterator_tag iterator_category;

			private:
				constexpr explicit arg_type_iterator(const data_t::handle *ptr) noexcept : type_data(ptr) {}

			public:
				constexpr arg_type_iterator() noexcept = default;

				constexpr arg_type_iterator operator++(int) noexcept
				{
					auto temp = *this;
					++type_data;
					return temp;
				}
				constexpr arg_type_iterator &operator++() noexcept
				{
					++type_data;
					return *this;
				}
				constexpr arg_type_iterator &operator+=(difference_type n) noexcept
				{
					type_data += n;
					return *this;
				}
				constexpr arg_type_iterator operator--(int) noexcept
				{
					auto temp = *this;
					--type_data;
					return temp;
				}
				constexpr arg_type_iterator &operator--() noexcept
				{
					--type_data;
					return *this;
				}
				constexpr arg_type_iterator &operator-=(difference_type n) noexcept
				{
					type_data -= n;
					return *this;
				}

				[[nodiscard]] constexpr arg_type_iterator operator+(difference_type n) const noexcept
				{
					return arg_type_iterator{type_data + n};
				}
				[[nodiscard]] constexpr arg_type_iterator operator-(difference_type n) const noexcept
				{
					return arg_type_iterator{type_data - n};
				}
				[[nodiscard]] constexpr difference_type operator-(const arg_type_iterator &other) const noexcept
				{
					return type_data - other.type_data;
				}

				[[nodiscard]] constexpr auto operator*() const noexcept { return type_info{*type_data}; }
				[[nodiscard]] constexpr auto operator[](difference_type i) const noexcept
				{
					return type_info{type_data[i]};
				}

				[[nodiscard]] constexpr auto operator<=>(const arg_type_iterator &) const noexcept = default;
				[[nodiscard]] constexpr bool operator==(const arg_type_iterator &) const noexcept = default;

			private:
				const data_t::handle *type_data;
			};

		public:
			typedef arg_type_iterator iterator;
			typedef arg_type_iterator const_iterator;
			typedef std::reverse_iterator<iterator> reverse_iterator;
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		private:
			constexpr explicit signature_info(data_t::handle ret, const data_t::handle *args, size_type args_n) noexcept
				: ret(ret), args(args), args_n(args_n)
			{
			}

		public:
			constexpr signature_info() noexcept = default;

			/** Returns the return type of the signature.
			 * If a signature does not have a return type (ex. if it is a constructor signature),
			 * returns an empty type info. */
			[[nodiscard]] constexpr type_info return_type() const noexcept { return type_info{ret}; }

			/** Returns the amount of arguments of the signature. */
			[[nodiscard]] constexpr size_type arg_count() const noexcept { return args_n; }
			/** @copydoc arg_count */
			[[nodiscard]] constexpr size_type size() const noexcept { return arg_count(); }
			/** Returns maximum amount of arguments of the signature. */
			[[nodiscard]] constexpr size_type max_size() const noexcept
			{
				return std::numeric_limits<size_type>::max();
			}

			/** Returns iterator to the first argument type of the signature. */
			[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{args}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last argument type of the signature. */
			[[nodiscard]] constexpr iterator end() const noexcept { return iterator{args + size()}; }
			/** @copydoc end */
			[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

			/** Returns reverse iterator to the last argument of the signature. */
			[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
			/** @copydoc rbegin */
			[[nodiscard]] constexpr reverse_iterator crbegin() const noexcept { return rbegin(); }
			/** Returns reverse iterator one past the first argument of the signature. */
			[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
			/** @copydoc rend */
			[[nodiscard]] constexpr reverse_iterator crend() const noexcept { return rend(); }

			/** Returns type info of the first argument of the signature. */
			[[nodiscard]] constexpr type_info front() const noexcept { return type_info{*args}; }
			/** Returns type info of the last argument of the signature. */
			[[nodiscard]] constexpr type_info back() const noexcept { return type_info{*(args + size() - 1)}; }
			/** Returns type info of the nth argument of the signature. */
			[[nodiscard]] constexpr type_info at(size_type n) const noexcept { return type_info{*(args + n)}; }
			/** @copydoc at */
			[[nodiscard]] constexpr type_info operator[](size_type n) const noexcept { return at(n); }

			[[nodiscard]] constexpr bool operator==(const signature_info &other) const noexcept
			{
				return return_type() == other.return_type() && std::equal(begin(), end(), other.begin(), other.end());
			}

		private:
			data_t::handle ret = {};
			const data_t::handle *args = nullptr;
			size_type args_n = 0;
		};
		/** @brief Structure containing information about a type's constructor. */
		class constructor_info
		{
			friend class type_info;
			friend class constructor_iterator;

			constexpr explicit constructor_info(const data_t::type_ctor *ptr) noexcept : ctor(ptr) {}

		public:
			constructor_info() = delete;

			/** Returns signature of the constructor.
			 * @note Returned signature will always have an empty return type. */
			[[nodiscard]] constexpr signature_info signature() const noexcept
			{
				return signature_info{data_t::handle{}, ctor->arg_types.data(), ctor->arg_types.size()};
			}

			/** Invokes constructor for the passed object & argument array.
			 *
			 * @param ptr Pointer to the object's memory.
			 * @param args Array of pointers to the arguments passed to the constructor.
			 *
			 * @warning Passed argument array must contain all arguments required by the constructor.
			 * Passing an invalid argument array will result in undefined behavior. */
			constexpr void invoke(void *ptr, const void *const args[]) const { ctor->invoke(ptr, args); }
			/** Invokes constructor for the passed object & arguments.
			 *
			 * @param ptr Pointer to the object's memory.
			 * @param args Arguments passed to the constructor. */
			template<typename... Args>
			constexpr void invoke(void *ptr, Args &&...args) const
			{
				if constexpr (sizeof...(args) != 0)
				{
					/* Make an array of pointers to args. */
					const void *args_array[sizeof...(args)] = {std::addressof(args)...};
					invoke(ptr, args_array);
				}
				else
					invoke(ptr, nullptr);
			}
			/** Invokes constructor for the passed object & arguments.
			 *
			 * @param ptr Pointer to the object's memory.
			 * @param args_begin Iterator to the first element of the `any` sequence containing arguments passed to the constructor.
			 * @param args_end Iterator one past the last `any` object of the argument sequence.
			 *
			 * @throw bad_type_exception If the argument sequence does not contain all arguments required by the constructor. */
			template<forward_iterator_for<any> Iter>
			constexpr void invoke(void *ptr, Iter args_begin, Iter args_end) const;
			/** Invokes constructor for the passed object & arguments.
			 *
			 * @param ptr Pointer to the object's memory.
			 * @param args Range of `any` objects containing arguments passed to the constructor.
			 *
			 * @throw bad_type_exception If the args range does not contain all arguments required by the constructor. */
			template<forward_range_for<any> R>
			constexpr void invoke(void *ptr, const R &args) const
			{
				invoke(ptr, std::ranges::begin(args), std::ranges::end(args));
			}

		private:
			const data_t::type_ctor *ctor;
		};
		/** @brief Structure containing information about a type's attribute. */
		class attribute_info
		{
			friend class type_info;
			friend class attribute_iterator;

			constexpr explicit attribute_info(const data_t::type_attribute *ptr) noexcept : attribute(ptr) {}

		public:
			attribute_info() = delete;

			/** Returns type info of the attribute's type. */
			[[nodiscard]] constexpr type_info type() const noexcept { return type_info{attribute->type}; }
			/** Returns type id of the attribute's type. */
			[[nodiscard]] constexpr type_id tid() const noexcept { return attribute->type->tid; }

			/** Returns void pointer to raw attribute data. */
			[[nodiscard]] constexpr const void *data() const noexcept { return attribute->data; }
			/** Returns `any` instance containing reference to the attribute data. */
			[[nodiscard]] constexpr any get() const noexcept;

		private:
			const data_t::type_attribute *attribute;
		};

	private:
		template<typename Node, typename Iterator, typename Handle>
		class type_data_view
		{
			friend class type_info;

		public:
			typedef Handle value_type;
			typedef Handle reference;
			typedef Handle const_reference;
			typedef Iterator iterator;
			typedef Iterator const_iterator;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

		private:
			using view_data_t = data_t::type_node_view<Node>;

			constexpr explicit type_data_view(view_data_t data) noexcept : data(std::move(data)) {}

		public:
			constexpr type_data_view() noexcept = default;

			/** Returns iterator to the start of the view. */
			[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{data.begin()}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator to the end of the view. */
			[[nodiscard]] constexpr iterator end() const noexcept { return iterator{data.end()}; }
			/** @copydoc end */
			[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

			/** Returns element at the front of the view. */
			[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }

			/** Returns the size of the view. */
			[[nodiscard]] constexpr size_type size() const noexcept { return data.size(); }
			/** Returns the max size of the view. */
			[[nodiscard]] constexpr size_type max_size() const noexcept { return data.max_size(); }
			/** Checks if the view is empty. */
			[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		private:
			view_data_t data;
		};

		class parent_iterator : data_t::type_node_iterator<data_t::type_parent>
		{
			using base_t = data_t::type_node_iterator<data_t::type_parent>;

		public:
			typedef type_info value_type;
			typedef type_info reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			constexpr explicit parent_iterator(base_t value) noexcept : base_t(value) {}

		public:
			constexpr parent_iterator() noexcept = default;

			constexpr parent_iterator operator++(int) noexcept
			{
				auto result = *this;
				base_t::move_next();
				return result;
			}
			constexpr parent_iterator &operator++() noexcept
			{
				base_t::move_next();
				return *this;
			}

			[[nodiscard]] constexpr auto operator*() const noexcept { return type_info{base_t::node->type}; }

			[[nodiscard]] constexpr bool operator==(const parent_iterator &) const noexcept = default;
		};
		class constructor_iterator : data_t::type_node_iterator<data_t::type_ctor>
		{
			using base_t = data_t::type_node_iterator<data_t::type_ctor>;

		public:
			typedef constructor_info value_type;
			typedef constructor_info reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			constexpr explicit constructor_iterator(base_t value) noexcept : base_t(value) {}

		public:
			constexpr constructor_iterator() noexcept = default;

			constexpr constructor_iterator operator++(int) noexcept
			{
				auto result = *this;
				base_t::move_next();
				return result;
			}
			constexpr constructor_iterator &operator++() noexcept
			{
				base_t::move_next();
				return *this;
			}

			[[nodiscard]] constexpr auto operator*() const noexcept { return constructor_info{base_t::node}; }

			[[nodiscard]] constexpr bool operator==(const constructor_iterator &) const noexcept = default;
		};
		class attribute_iterator : data_t::type_node_iterator<data_t::type_attribute>
		{
			using base_t = data_t::type_node_iterator<data_t::type_attribute>;

		public:
			typedef attribute_info value_type;
			typedef attribute_info reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			constexpr explicit attribute_iterator(base_t value) noexcept : base_t(value) {}

		public:
			constexpr attribute_iterator() noexcept = default;

			constexpr attribute_iterator operator++(int) noexcept
			{
				auto result = *this;
				base_t::move_next();
				return result;
			}
			constexpr attribute_iterator &operator++() noexcept
			{
				base_t::move_next();
				return *this;
			}

			[[nodiscard]] constexpr auto operator*() const noexcept { return attribute_info{base_t::node}; }

			[[nodiscard]] constexpr bool operator==(const attribute_iterator &) const noexcept = default;
		};

		using parent_view = type_data_view<data_t::type_parent, parent_iterator, type_info>;
		using constructor_view = type_data_view<data_t::type_ctor, constructor_iterator, constructor_info>;
		using attribute_view = type_data_view<data_t::type_attribute, attribute_iterator, attribute_info>;

	private:
		constexpr explicit type_info(data_t::handle data) noexcept : data(data) {}

	public:
		constexpr type_info() noexcept = default;

		/** Checks if the type info is empty (does not describe any type). */
		[[nodiscard]] constexpr bool empty() const noexcept { return data.empty(); }

		/** Returns id of the type. */
		[[nodiscard]] constexpr type_id tid() const noexcept { return data->tid; }
		/** Returns name of the type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return data->tid.name(); }
		/** Returns hash of the type. */
		[[nodiscard]] constexpr std::size_t hash() const noexcept { return data->tid.hash(); }

		/** Returns the type's size. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return data->size; }
		/** Returns the type's alignment. */
		[[nodiscard]] constexpr std::size_t alignment() const noexcept { return data->alignment; }

		/** Checks if the type is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return data->variant_type & data_t::VARIANT_CONST; }
		/** Checks if the type is volatile-qualified. */
		[[nodiscard]] constexpr bool is_volatile() const noexcept
		{
			return data->variant_type & data_t::VARIANT_VOLATILE;
		}
		/** Checks if the type is cv-qualified. */
		[[nodiscard]] constexpr bool is_cv() const noexcept
		{
			return data->variant_type == data_t::VARIANT_CONST_VOLATILE;
		}
		/** Checks if the type is a qualified variant of another type.
		 * @return true if the type is a qualified variant, false otherwise. */
		[[nodiscard]] constexpr bool is_variant() const noexcept
		{
			return !data->variants[data_t::VARIANT_PARENT].empty();
		}
		/** If the type is a qualified variant, returns the unqualified "parent" type.
		 * Otherwise, returns an empty type info. */
		[[nodiscard]] constexpr type_info get_variant_parent() const noexcept
		{
			return type_info{data->variants[data_t::VARIANT_PARENT]};
		}
		/** Checks if the type has a const-qualified variant.
		 * @return true if the type has a const-qualified variant, false otherwise.
		 * @note If the type itself is const-qualified, it does not have a const-qualified variant. */
		[[nodiscard]] constexpr bool has_const_variant() const noexcept
		{
			return !data->variants[data_t::VARIANT_CONST].empty();
		}
		/** Returns type info of the const-qualified variant of the type.
		 * @return Type info of the const-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is const-qualified, it does not have a const-qualified variant. */
		[[nodiscard]] constexpr type_info get_const_variant() const noexcept
		{
			return type_info{data->variants[data_t::VARIANT_CONST]};
		}
		/** Checks if the type has a volatile-qualified variant.
		 * @return true if the type has a volatile-qualified variant, false otherwise.
		 * @note If the type itself is volatile-qualified, it does not have a volatile-qualified variant. */
		[[nodiscard]] constexpr bool has_volatile_variant() const noexcept
		{
			return !data->variants[data_t::VARIANT_VOLATILE].empty();
		}
		/** Returns type info of the volatile-qualified variant of the type.
		 * @return Type info of the volatile-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is volatile-qualified, it does not have a volatile-qualified variant. */
		[[nodiscard]] constexpr type_info get_volatile_variant() const noexcept
		{
			return type_info{data->variants[data_t::VARIANT_VOLATILE]};
		}
		/** Checks if the type has a cv-qualified variant.
		 * @return true if the type has a cv-qualified variant, false otherwise.
		 * @note If the type itself is either const or volatile-qualified, it does not have a cv-qualified variant. */
		[[nodiscard]] constexpr bool has_cv_variant() const noexcept
		{
			return !data->variants[data_t::VARIANT_CONST_VOLATILE].empty();
		}
		/** Returns type info of the cv-qualified variant of the type.
		 * @return Type info of the cv-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is either const or volatile-qualified, it does not have a cv-qualified variant. */
		[[nodiscard]] constexpr type_info get_cv_variant() const noexcept
		{
			return type_info{data->variants[data_t::VARIANT_CONST_VOLATILE]};
		}
		/** Checks if the type has a variant of a specific type.
		 * @param id Id of the variant type.
		 * @return true if the type has a variant of `id` type, false otherwise. */
		[[nodiscard]] constexpr bool has_variant(type_id id) const noexcept
		{
			return (has_const_variant() && get_const_variant().tid() == id) ||
				   (has_volatile_variant() && get_volatile_variant().tid() == id) ||
				   (has_cv_variant() && get_cv_variant().tid() == id);
		}
		/** Checks if the type has a variant of a specific type.
		 * @tparam T Variant type.
		 * @return true if the type has a variant of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] constexpr bool has_variant() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return has_variant(id);
		}

		/** Returns a view containing parents of the type. */
		[[nodiscard]] constexpr parent_view get_parents() const noexcept
		{
			return parent_view{data->get_parent_view()};
		}
		/** Checks if the type has a parent of a specific type.
		 * @param id Id of the parent type.
		 * @return true if the type has a parent of `id` type, false otherwise. */
		[[nodiscard]] constexpr bool has_parent(type_id id) const noexcept { return data->has_parent(id); }
		/** Checks if the type has a parent of a specific type.
		 * @tparam T Parent type.
		 * @return true if the type has a parent of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] constexpr bool has_parent() const noexcept
		{
			return data->template has_parent<T>();
		}

		/** Checks if the type is compatible with another type.
		 *
		 * Type `A` is considered compatible with type `B` if `A` == `B`, if `B` is a variant of `A`,
		 * or if `B` is a parent of `A`. If `A` is compatible with `B`.
		 * Reference of type `A` can be safely cast to a reference of type `B`.
		 *
		 * @param id Id of the type to check for compatibility with.
		 * @return true if the type is compatible with `id` type, false otherwise. */
		[[nodiscard]] constexpr bool compatible_with(type_id id) const noexcept
		{
			return tid() == id || has_variant(id) || has_parent(id);
		}
		/** Checks if the type is compatible with a specific type.
		 *
		 * Type `A` is considered compatible with type `B` if `A` == `B`, if `B` is a variant of `A`,
		 * or if `B` is a parent of `A`. If `A` is compatible with `B`.
		 * Reference of type `A` can be safely cast to a reference of type `B`.
		 *
		 * @tparam T Type to check for compatibility with.
		 * @return true if the type is compatible with `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] constexpr bool compatible_with() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return compatible_with(id);
		}

		/** Returns a view containing attributes of the type. */
		[[nodiscard]] constexpr attribute_view get_attributes() const noexcept
		{
			return attribute_view{data->get_attribute_view()};
		}
		/** Checks if the type has an attribute of a specific type.
		 * @param id Id of the attribute's type.
		 * @return true if the type has an attribute of `id` type, false otherwise. */
		[[nodiscard]] constexpr bool has_attribute(type_id id) const noexcept { return data->has_attribute(id); }
		/** Checks if the type has an attribute of a specific type.
		 * @tparam T Attribute's type.
		 * @return true if the type has an attribute of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] constexpr bool has_attribute() const noexcept
		{
			return data->template has_attribute<T>();
		}
		/** Returns attribute of a specific type.
		 * @tparam T Attribute's type.
		 * @return Pointer to attribute's data if such attribute is present, nullptr otherwise. */
		template<typename T>
		[[nodiscard]] constexpr const T *get_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			if (auto node = data->get_attribute(id); node != nullptr) [[likely]]
				return static_cast<const T *>(node->data);
			else
				return nullptr;
		}

		/** Returns a view containing constructors of the type. */
		[[nodiscard]] constexpr constructor_view get_constructors() const noexcept
		{
			return constructor_view{data->get_ctor_view()};
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @param args_first Iterator to the start of the argument type sequence.
		 * @param args_last Iterator to the end of the argument type sequence.
		 * @return true if the type is constructible, false otherwise. */
		template<forward_iterator_for<type_info> I>
		[[nodiscard]] constexpr bool constructible_with(I args_first, I args_last) const
		{
			return data->has_ctor(type_info_iterator{args_first}, type_info_iterator{args_last});
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @param args Range containing argument types.
		 * @return true if the type is constructible, false otherwise. */
		template<forward_range_for<type_info> R>
		[[nodiscard]] constexpr bool constructible_with(const R &args) const
		{
			return constructible_with(std::ranges::begin(args), std::ranges::end(args));
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @tparam Args Arguments of the constructor.
		 * @return true if the type is constructible, false otherwise. */
		template<typename... Args>
		[[nodiscard]] constexpr bool constructible_with() const noexcept
		{
			return data->template has_ctor<Args...>();
		}

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			if (!empty() && !other.empty()) [[likely]]
				return data == other.data;
			else
				return empty() == other.empty();
		}

	private:
		/** Handle to type's type_data instance. */
		data_t::handle data = {};
	};
}	 // namespace sek

/** Exports type info for a specific type.
 * This is needed in order to correctly link type factories & generated type data across translation units.
 *
 * @note It is generally not recommended to export qualified versions of a type (instead export the unqualified type).
 * @warning If a type is exported, a factory must be explicitly defined inside a single translation unit,
 * failing to do so will cause link errors.
 * @note Setting a custom type id via `SEK_SET_TYPE_ID` is not required, but is recommended.
 *
 * @example
 * ```cpp
 * // All translation units that include this header will link against the same type data & factory for `my_type`.
 * SEK_EXPORT_TYPE(my_type)
 * ``` */
#define SEK_EXPORT_TYPE(T)                                                                                             \
	template<>                                                                                                         \
	constexpr bool sek::detail::is_exported_type<T> = true;                                                            \
	extern template struct SEK_API_IMPORT sek::detail::type_data::instance<T>;

#define SEK_DECLARE_TYPE_2(T, name)                                                                                    \
	SEK_SET_TYPE_ID(T, name)                                                                                           \
	SEK_EXPORT_TYPE(T)
#define SEK_DECLARE_TYPE_1(T) SEK_DECLARE_TYPE_2(T, #T)

/** Declares a type with a custom id & exports it's type info.
 * Equivalent to using `SEK_SET_TYPE_ID` followed by `SEK_EXPORT_TYPE`.
 *
 * @note Type name is optional, if not specified will use a string made from the first argument.
 * @warning Every declared type must have a type factory defined inside a single translation unit (via
 * `SEK_TYPE_FACTORY`), a missing type factory will result in link errors.
 *
 * @example
 * ```cpp
 * // Type `some_namespace::some_type` will be identifiable as `my_type` and it's factory will be exported.
 * SEK_DECLARE_TYPE(some_namespace::some_type, "my_type")
 * ```
 * @example
 * ```cpp
 * // Type `some_other_type` will be identifiable as `some_other_type` and it's factory will be exported.
 * SEK_DECLARE_TYPE(some_other_type)
 * ```  */
#define SEK_DECLARE_TYPE(T, ...)                                                                                       \
	SEK_GET_MACRO_2(T, (__VA_ARGS__), SEK_DECLARE_TYPE_2, SEK_DECLARE_TYPE_1)(T, (__VA_ARGS__))

/* Creates a type factory for the specific type. Type factories are invoked on static initialization.
 *
 * @warning Types that have a factory must be exported via `SEK_EXPORT_TYPE`,
 * failing to do so may result in compilation/linking errors. */
#define SEK_TYPE_FACTORY(T)                                                                                                  \
	static_assert(sek::detail::is_exported_type<T>, "Type must be exported for type factory to work & be linked correctly"); \
	template struct SEK_API_EXPORT sek::detail::type_data::instance<T>;                                                      \
	namespace                                                                                                                \
	{                                                                                                                        \
		template<typename U>                                                                                                 \
		struct sek_type_factory : sek::detail::type_factory_base<U>                                                          \
		{                                                                                                                    \
			static void invoke() noexcept;                                                                                   \
                                                                                                                             \
			static const sek_type_factory instance;                                                                          \
                                                                                                                             \
			sek_type_factory() noexcept { invoke(); }                                                                        \
		};                                                                                                                   \
		template<typename U>                                                                                                 \
		const sek_type_factory<U> sek_type_factory<U>::instance = {};                                                        \
                                                                                                                             \
		template<>                                                                                                           \
		void sek_type_factory<T>::invoke() noexcept;                                                                         \
		template struct sek_type_factory<T>;                                                                                 \
	}                                                                                                                        \
	template<>                                                                                                               \
	void sek_type_factory<T>::invoke() noexcept
