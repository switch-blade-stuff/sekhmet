//
// Created by switchblade on 2022-02-04.
//

#pragma once

#include <cstdarg>
#include <ranges>
#include <vector>

#include "assert.hpp"
#include "engine_exception.hpp"
#include "meta_containers.hpp"
#include "type_id.hpp"

namespace sek::detail
{
	class any;
	class any_ref;

	/** @brief Exception thrown when a type is invalid or incompatible with another. */
	class bad_type_exception : engine_exception
	{
	public:
		bad_type_exception() noexcept = default;
		explicit bad_type_exception(const char *msg) noexcept;
		explicit bad_type_exception(type_id type) noexcept;
		bad_type_exception(type_id from, type_id to) noexcept;
		~bad_type_exception() override;

		[[nodiscard]] const char *what() const noexcept override { return msg; }

	private:
		const char *msg = nullptr;
	};

	struct SEK_API type_data
	{
		enum flags_t
		{
			FLAG_NONE = 0,
			FLAG_CONST = 0b1,
			FLAG_VOLATILE = 0b10,
			FLAG_CV = FLAG_CONST | FLAG_VOLATILE,
		};

		template<typename T>
		struct instance
		{
			constexpr type_data operator()() const noexcept;

			constinit static type_data value;
		};

		template<typename Child>
		struct node_base
		{
			constexpr void link(const Child *&next_ptr) noexcept
			{
				next = next_ptr;
				next_ptr = static_cast<const Child *>(this);
			}
			const Child *next;
		};
		struct type_node : node_base<type_node>
		{
			constexpr explicit type_node(const type_data *type) noexcept : node_base(), type(type) {}

			template<typename T>
			struct instance
			{
				constexpr type_node operator()() const noexcept { return type_node{&type_data::instance<T>::value}; }

				constinit static type_node value;
			};

			const type_data *type;
		};
		struct data_node : node_base<data_node>
		{
			template<typename T>
			constexpr explicit data_node(const T &value) noexcept
				: node_base(), data(&value), type(&type_data::instance<T>::value)
			{
			}

			template<auto V>
			struct instance
			{
				constexpr data_node operator()() const noexcept { return data_node{auto_constant<V>::value}; }

				constinit static data_node value;
			};

			template<typename T>
			[[nodiscard]] constexpr const T *as() const noexcept
			{
				return static_cast<const T *>(data);
			}

			const void *data;
			const type_data *type;
		};
		struct ctor_node : node_base<ctor_node>
		{
			typedef void (*ctor_proxy)(void *, std::va_list);

			constexpr explicit ctor_node(ctor_proxy proxy, meta_view<const type_data *> args) noexcept
				: node_base(), proxy(proxy), arg_types(args)
			{
			}

			template<typename T, typename... Args>
			struct instance
			{
				template<typename U>
				constexpr static decltype(auto) expand_args(std::va_list args)
				{
					return va_arg(args, U);
				}
				constexpr static meta_view<const type_data *> get_arg_array()
				{
					constexpr auto &arg_array = array_constant<const type_data *, &type_data::instance<Args>::value...>::value;
					return meta_view<const type_data *>{arg_array};
				}

				constexpr ctor_node operator()() const noexcept
				{
					constexpr auto proxy_func = [](void *obj, [[maybe_unused]] std::va_list args)
					{
						auto ptr = static_cast<T *>(obj);
						std::construct_at(ptr, expand_args<Args>(args)...);
					};
					return ctor_node{proxy_func, get_arg_array()};
				}

				constinit static ctor_node value;
			};

			void invoke(void *ptr, ...) const
			{
				std::va_list args_list;
				va_start(args_list, ptr);

				proxy(ptr, args_list);

				va_end(args_list);
			}

			const ctor_proxy proxy;
			meta_view<const type_data *> arg_types;
		};

		type_id tid;
		std::size_t size;
		std::size_t alignment;

		flags_t flags;
		const type_data *variants[3];

		const type_node *parents;
		const data_node *attributes;
		void (*destructor)(void *);
		const ctor_node *constructors;
	};
	template<typename T>
	constinit type_data::type_node type_data::type_node::instance<T>::value = instance<T>{}();
	template<auto V>
	constinit type_data::data_node type_data::data_node::instance<V>::value = instance<V>{}();
	template<typename T, typename... Args>
	constinit type_data::ctor_node type_data::ctor_node::instance<T, Args...>::value = instance<T, Args...>{}();
	template<typename T>
	constexpr type_data type_data::instance<T>::operator()() const noexcept
	{
		type_data data = {
			.tid = type_id::identify<T>(),
			.size = sizeof(T),
			.alignment = alignof(T),
			.flags = FLAG_NONE,
			.variants = {nullptr},
			.parents = nullptr,
			.attributes = nullptr,
			.destructor = nullptr,
			.constructors = nullptr,
		};

		/* If the type is default constructible & destructible, set constructor & destructor. */
		if constexpr (std::is_object_v<T> && std::is_destructible_v<T>)
		{
			data.destructor = +[](void *obj) { std::destroy_at(static_cast<T *>(obj)); };
			if constexpr (std::is_default_constructible_v<T>) data.constructors = &ctor_node::instance<T>::value;
		}

		/* Set flags & add variants. */
		if constexpr (std::is_const_v<T>)
			data.flags = static_cast<flags_t>(data.flags | FLAG_CONST);
		else
			data.variants[0] = &instance<std::add_const_t<T>>::value;
		if constexpr (std::is_volatile_v<T>)
			data.flags = static_cast<flags_t>(data.flags | FLAG_VOLATILE);
		else
			data.variants[1] = &instance<std::add_volatile_t<T>>::value;

		if constexpr (!std::is_const_v<T> && !std::is_volatile_v<T>)
			data.variants[2] = &instance<std::add_cv_t<T>>::value;

		return data;
	}
	template<typename T>
	constinit type_data type_data::instance<T>::value = instance<T>{}();

	template<typename T>
	class type_factory_base
	{
	private:
		constexpr static type_data &data() noexcept { return type_data::instance<T>::value; }

		template<typename U>
		[[nodiscard]] static bool has_parent() noexcept
		{
			for (auto parent = data().parents; parent != nullptr; parent = parent->next)
				if (parent->type->tid == type_id::identify<U>()) return true;
			return false;
		}
		template<typename U, typename... Us>
		static void parents_impl() noexcept requires std::is_base_of_v<U, T>
		{
			if (!has_parent<U>()) [[likely]]
			{
				constexpr auto &parent = type_data::type_node::instance<U>::value;
				parent.link(data().parents);
			}

			if constexpr (sizeof...(Us) != 0) parents_impl<Us...>();
		}

		template<auto V, auto... Vals>
		static void attributes_impl() noexcept
		{
			constexpr auto &attribute = type_data::data_node::instance<V>::value;
			attribute.link(data().attributes);

			if constexpr (sizeof...(Vals) != 0) attributes_impl<Vals...>();
		}

		template<typename... Args>
		[[nodiscard]] static bool has_ctor() noexcept
		{
			auto args = type_data::ctor_node::instance<T, Args...>::get_arg_array();
			for (auto ctor = data().constructors; ctor != nullptr; ctor = ctor->next)
				if (std::equal(args.begin(), args.end(), ctor->arg_types.begin(), ctor->arg_types.end())) return true;
			return false;
		}
		template<typename... Args>
		static void constructor_impl() noexcept
		{
			if (!has_ctor<Args...>()) [[likely]]
			{
				constexpr auto &ctor = type_data::ctor_node::instance<T, Args...>::value;
				ctor.link(data().constructors);
			}
		}

	protected:
		/** Adds the specified parents to the type.
		 * @tparam Ts Types to add as parents.
		 * @note Parent can only be added once. */
		template<typename... Ts>
		static void parents() noexcept
		{
			parents_impl<Ts...>();
		}
		/** Adds the specified attributes to the type.
		 * @tparam Vals Values to add as attributes. */
		template<auto... Vals>
		static void attributes() noexcept
		{
			attributes_impl<Vals...>();
		}

		/** Adds a constructor to the type.
		 * @tparam Args Arguments of the constructor.
		 * @note If the type is default-constructible, the default constructor will be implicitly added.
		 * @note In order to pass arguments by reference, use a pointer or a reference wrapper type (ex. `sek::std::reference_wrapper`). */
		template<typename... Args>
		static void constructor() noexcept requires std::constructible_from<T, Args...> && std::is_destructible_v<T>
		{
			constructor_impl<Args...>();
		}
	};

	/** @brief Structure used to represent information about a type. */
	class SEK_API type_info
	{
	public:
		/** Adds a type to runtime lookup database.
		 * @param type Type info of the type to add to runtime database.
		 * @return true if a type was added successfully, false otherwise.
		 * @note This function will fail if a type with the same id was already registered. */
		static bool register_type(type_info type);
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
		static bool deregister_type(type_info type);
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
			return type_info{&type_data::instance<T>::value};
		}
		/** Looks up a type within the runtime lookup database.
		 * @tparam tid Id of the type to search for.
		 * @return `type_info` instance for the requested type. If an invalid tid was specified, returns an invalid type info.
		 * @note Type must be registered for it to be available. */
		[[nodiscard]] static type_info get(type_id tid) noexcept;
		/** Returns vector containing all currently registered types. */
		[[nodiscard]] static std::vector<type_info> all();

	private:
		constexpr explicit type_info(const detail::type_data *data) noexcept : data(data) {}

		/** @brief Container view used to read type's variant array. */
		class variant_view
		{
			friend class type_info;

			/** @brief Iterator used to iterate over a type's variant array. */
			class variant_iterator
			{
				friend class variant_view;

			public:
				typedef type_info value_type;

				typedef std::size_t size_type;
				typedef std::ptrdiff_t difference_type;
				typedef std::bidirectional_iterator_tag iterator_category;

			private:
				constexpr explicit variant_iterator(const type_data *const *ptr) noexcept : type_ptr(ptr) {}

			public:
				constexpr variant_iterator() noexcept = default;

				constexpr variant_iterator operator++(int) noexcept
				{
					auto temp = *this;
					++(*this);
					return temp;
				}
				constexpr variant_iterator &operator++() noexcept
				{
					type_ptr++;
					return *this;
				}
				constexpr variant_iterator operator--(int) noexcept
				{
					auto temp = *this;
					--(*this);
					return temp;
				}
				constexpr variant_iterator &operator--() noexcept
				{
					type_ptr--;
					return *this;
				}

				[[nodiscard]] constexpr bool operator==(const variant_iterator &) const noexcept = default;

				[[nodiscard]] constexpr value_type operator*() const noexcept { return type_info{*type_ptr}; }

				constexpr void swap(variant_iterator &other) noexcept
				{
					using std::swap;
					swap(type_ptr, other.type_ptr);
				}
				friend constexpr void swap(variant_iterator &a, variant_iterator &b) noexcept { a.swap(b); }

			private:
				const type_data *const *type_ptr = nullptr;
			};

		public:
			typedef type_info value_type;
			typedef variant_iterator iterator;
			typedef iterator const_iterator;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

		private:
			constexpr explicit variant_view(const type_data *const *data) noexcept : data(data) {}

		public:
			variant_view() = delete;

			[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{data}; }
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr iterator end() const noexcept
			{
				return iterator{data + SEK_ARRAY_SIZE(type_data::variants)};
			}
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
			[[nodiscard]] constexpr explicit operator bool() const noexcept { return !empty(); }

			[[nodiscard]] constexpr auto front() const noexcept { return *begin(); }
			[[nodiscard]] constexpr size_type size() const noexcept
			{
				return static_cast<size_type>(std::distance(begin(), end()));
			}

		private:
			const type_data *const *data;
		};

		template<typename NodeT>
		class type_data_iterator
		{
			using node_type = NodeT;

		public:
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		protected:
			constexpr explicit type_data_iterator(const node_type *ptr) noexcept : node(ptr) {}

		public:
			constexpr type_data_iterator() noexcept = default;

			[[nodiscard]] constexpr bool operator==(const type_data_iterator &) const noexcept = default;

			constexpr void swap(type_data_iterator &other) noexcept
			{
				using std::swap;
				swap(node, other.node);
			}

		protected:
			constexpr type_data_iterator &increment() noexcept
			{
				node = node->next;
				return *this;
			}

			const node_type *node;
		};

		template<typename Iterator>
		class type_data_view
		{
			friend class type_info;

		public:
			typedef type_info value_type;
			typedef Iterator iterator;
			typedef iterator const_iterator;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

		private:
			constexpr explicit type_data_view(const auto *node) noexcept : first(node) {}

		public:
			type_data_view() = delete;

			[[nodiscard]] constexpr iterator begin() const noexcept { return first; }
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr iterator end() const noexcept { return iterator{}; }
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
			[[nodiscard]] constexpr explicit operator bool() const noexcept { return !empty(); }

			[[nodiscard]] constexpr auto front() const noexcept { return *begin(); }
			[[nodiscard]] constexpr size_type size() const noexcept { return std::distance(begin(), end()); }

		private:
			iterator first;
		};

		/** @brief Iterator used to iterate over a type's parent list. */
		class parent_iterator : public type_data_iterator<type_data::type_node>
		{
			using base = type_data_iterator<type_data::type_node>;

			template<typename>
			friend class type_data_view;

		public:
			typedef type_info value_type;

			using type_data_iterator<type_data::type_node>::swap;

		private:
			constexpr explicit parent_iterator(const type_data::type_node *ptr) noexcept : base(ptr) {}

		public:
			constexpr parent_iterator() noexcept = default;

			constexpr parent_iterator operator++(int) noexcept
			{
				auto temp = *this;
				base::increment();
				return temp;
			}
			constexpr parent_iterator &operator++() noexcept
			{
				base::increment();
				return *this;
			}

			[[nodiscard]] constexpr value_type operator*() const noexcept { return type_info{node->type}; }

			friend constexpr void swap(parent_iterator &a, parent_iterator &b) noexcept { a.swap(b); }
		};
		/** @brief Iterator used to iterate over a type's attribute list. */
		class attribute_iterator : public type_data_iterator<type_data::data_node>
		{
			using base = type_data_iterator<type_data::data_node>;

			template<typename>
			friend class type_data_view;

		public:
			typedef any_ref value_type;

			using type_data_iterator<type_data::data_node>::swap;

		private:
			constexpr explicit attribute_iterator(const type_data::data_node *ptr) noexcept : base(ptr) {}

		public:
			constexpr attribute_iterator() noexcept = default;

			constexpr attribute_iterator operator++(int) noexcept
			{
				auto temp = *this;
				base::increment();
				return temp;
			}
			constexpr attribute_iterator &operator++() noexcept
			{
				base::increment();
				return *this;
			}

			[[nodiscard]] constexpr value_type operator*() const noexcept;

			friend constexpr void swap(attribute_iterator &a, attribute_iterator &b) noexcept { a.swap(b); }
		};

	public:
		constexpr type_info() noexcept = default;

		/** Checks if the type info is valid. */
		[[nodiscard]] constexpr bool valid() const noexcept { return data != nullptr; }

		/** Returns id of the type. */
		[[nodiscard]] type_id tid() const noexcept { return data->tid; }
		/** Returns name of the type. */
		[[nodiscard]] std::string_view name() const noexcept { return data->tid.name(); }
		/** Returns hash of the type. */
		[[nodiscard]] std::size_t hash() const noexcept { return data->tid.hash(); }

		/** Checks if the type is const-qualified. */
		[[nodiscard]] bool is_const() const noexcept { return data->flags & type_data::FLAG_CONST; }
		/** Checks if the type is volatile-qualified. */
		[[nodiscard]] bool is_volatile() const noexcept { return data->flags & type_data::FLAG_VOLATILE; }
		/** Checks if the type is cv-qualified. */
		[[nodiscard]] bool is_cv() const noexcept { return data->flags == type_data::FLAG_CV; }

		/** Returns the type's size. */
		[[nodiscard]] std::size_t size() const noexcept { return data->size; }
		/** Returns the type's alignment. */
		[[nodiscard]] std::size_t alignment() const noexcept { return data->alignment; }

		/** Returns a container view to variants of the type. */
		[[nodiscard]] auto variants() const noexcept { return variant_view{data->variants}; }
		/** Checks if the type has a variant of a specific type.
		 * @param id Id of the variant type.
		 * @return true if the type has a variant of `id` type, false otherwise. */
		[[nodiscard]] bool has_variant(type_id id) const noexcept
		{
			return (has_const_variant() && get_const_variant().tid() == id) ||
				   (has_volatile_variant() && get_volatile_variant().tid() == id) ||
				   (has_cv_variant() && get_cv_variant().tid() == id);
		}
		/** Checks if the type has a variant of a specific type.
		 * @tparam T Variant type.
		 * @return true if the type has a variant of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool has_variant() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return has_variant(id);
		}

		/** Checks if the type has a const-qualified variant.
		 * @return true if the type has a const-qualified variant, false otherwise.
		 * @note If the type itself is const-qualified, it does not have a const-qualified variant. */
		[[nodiscard]] bool has_const_variant() const noexcept { return data->variants[0] != nullptr; }
		/** Returns type info of the const-qualified variant of the type.
		 * @return Type info of the const-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is const-qualified, it does not have a const-qualified variant. */
		[[nodiscard]] type_info get_const_variant() const noexcept { return type_info{data->variants[0]}; }

		/** Checks if the type has a volatile-qualified variant.
		 * @return true if the type has a volatile-qualified variant, false otherwise.
		 * @note If the type itself is volatile-qualified, it does not have a volatile-qualified variant. */
		[[nodiscard]] bool has_volatile_variant() const noexcept { return data->variants[1] != nullptr; }
		/** Returns type info of the volatile-qualified variant of the type.
		 * @return Type info of the volatile-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is volatile-qualified, it does not have a volatile-qualified variant. */
		[[nodiscard]] type_info get_volatile_variant() const noexcept { return type_info{data->variants[1]}; }

		/** Checks if the type has a cv-qualified variant.
		 * @return true if the type has a cv-qualified variant, false otherwise.
		 * @note If the type itself is either const or volatile-qualified, it does not have a cv-qualified variant. */
		[[nodiscard]] bool has_cv_variant() const noexcept { return data->variants[2] != nullptr; }
		/** Returns type info of the cv-qualified variant of the type.
		 * @return Type info of the cv-qualified variant if such variant is present, empty type info otherwise.
		 * @note If the type itself is either const or volatile-qualified, it does not have a cv-qualified variant. */
		[[nodiscard]] type_info get_cv_variant() const noexcept { return type_info{data->variants[2]}; }

		/** Returns a container view to parents of the type. */
		[[nodiscard]] auto parents() const noexcept { return type_data_view<parent_iterator>{data->parents}; }
		/** Checks if the type has a parent of a specific type.
		 * @param id Id of the parent type.
		 * @return true if the type has a parent of `id` type, false otherwise. */
		[[nodiscard]] bool has_parent(type_id id) const noexcept
		{
			auto pred = [id](auto type) noexcept { return type.tid() == id; };
			return std::ranges::any_of(parents(), pred);
		}
		/** Checks if the type has a parent of a specific type.
		 * @tparam T Parent type.
		 * @return true if the type has a parent of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool has_parent() const noexcept
		{
			return has_parent(type_id::identify<T>());
		}

		/** Checks if the type is compatible with a specific type.
		 * Type `A` is considered compatible with type `B` if reference of type `A`
		 * can be implicitly cast to a reference of type `B`.
		 * @param id Id of the type to check for compatibility with.
		 * @return true if the type is compatible with `id` type, false otherwise. */
		[[nodiscard]] bool is_compatible(type_id id) const noexcept
		{
			return tid() == id || has_variant(id) || has_parent(id);
		}
		/** Checks if the type is compatible with a specific type.
		 * Type `A` is considered compatible with type `B` if reference of type `A`
		 * can be implicitly cast to a reference of type `B`.
		 * @tparam T Type to check for compatibility with.
		 * @return true if the type is compatible with `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool is_compatible() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return is_compatible(id);
		}

		/** Returns a container view to attributes of the type. */
		[[nodiscard]] auto attributes() const noexcept { return type_data_view<attribute_iterator>{data->attributes}; }
		/** Checks if the type has an attribute of a specific type.
		 * @param id Id of the attribute's type.
		 * @return true if the type has an attribute of `id` type, false otherwise. */
		[[nodiscard]] bool has_attribute(type_id id) const noexcept
		{
			for (auto attr = data->attributes; attr != nullptr; attr = attr->next)
				if (attr->type->tid == id) return true;
			return false;
		}
		/** Checks if the type has an attribute of a specific type.
		 * @tparam T Attribute's type.
		 * @return true if the type has an attribute of `T`, false otherwise. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			return has_attribute(id);
		}
		/** Returns attribute of a specific type.
		 * @param id Id of the attribute's type.
		 * @return `any_ref` instance referencing the attribute if such attribute is present. Otherwise, and empty `any_ref` instance. */
		[[nodiscard]] any_ref get_attribute(type_id id) const noexcept;
		/** Returns attribute of a specific type.
		 * @tparam T Attribute's type.
		 * @return Pointer to attribute's data if such attribute is present, nullptr otherwise. */
		template<typename T>
		[[nodiscard]] const T *get_attribute() const noexcept
		{
			constexpr auto id = type_id::identify<T>();
			for (auto attr = data->attributes; attr != nullptr; attr = attr->next)
				if (attr->type->tid == id) return static_cast<const T *>(attr->data);
			return nullptr;
		}

		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @param args_first Iterator to the start of the argument type sequence.
		 * @param args_first Iterator to the end of the argument type sequence.
		 * @return true if the type is constructible, false otherwise. */
		template<typename I>
		[[nodiscard]] bool constructible_with(I args_first, I args_last)
			const requires std::forward_iterator<I> && std::same_as<std::iter_value_t<I>, type_info>
		{
			return get_constructor_impl(args_first, args_last) != nullptr;
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @param args Range containing argument types.
		 * @return true if the type is constructible, false otherwise. */
		template<std::ranges::forward_range R>
		[[nodiscard]] bool constructible_with(const R &args) const
		{
			return constructible_with(std::ranges::begin(args), std::ranges::end(args));
		}
		/** Checks if the type has a constructor invocable with the specified argument types.
		 * @tparam Args Arguments of the constructor.
		 * @return true if the type is constructible, false otherwise. */
		template<typename... Args>
		[[nodiscard]] bool constructible_with() const noexcept
		{
			return get_constructor_impl<Args...>() != nullptr;
		}

		/** Constructs an object of this type, using a constructor with matching arguments.
		 * @param ptr Pointer to the object's memory.
		 * @param args Arguments passed to the constructor.
		 * @throw bad_type_exception If the type does not have a constructor invocable with the specific arguments. */
		template<typename... Args>
		void construct(void *ptr, Args... args) const
		{
			SEK_ASSERT(ptr != nullptr);

			const type_data::ctor_node *node = get_constructor_impl<Args...>();
			if (!node) [[unlikely]]
				throw bad_type_exception("Failed to find a matching constructor");
			node->invoke(ptr, std::forward<Args>(args)...);
		}
		/** Destroys an object of this type.
		 * @param ptr Pointer to the object's memory.
		 * @throw bad_type_exception If the type does not have a destructor. */
		void destroy(void *ptr) const
		{
			SEK_ASSERT(ptr != nullptr);

			auto dtor = data->destructor;
			if (!dtor) [[unlikely]]
				throw bad_type_exception("Type is not destructible");
			dtor(ptr);
		}

	private:
		template<typename... Args>
		[[nodiscard]] const type_data::ctor_node *get_constructor_impl() const noexcept
		{
			if constexpr (sizeof...(Args))
			{
				constexpr type_info arg_types[] = {type_info{&type_data::instance<Args>::value}...};
				return get_constructor_impl(std::begin(arg_types), std::end(arg_types));
			}
			else
				return get_constructor_impl<const type_info *>(nullptr, nullptr);
		}
		template<typename I>
		[[nodiscard]] const type_data::ctor_node *get_constructor_impl(I args_first, I args_last) const
		{
			for (auto ctor = data->constructors; ctor != nullptr; ctor = ctor->next)
			{
				constexpr auto pred = [](type_info info, const type_data *d) { return info.tid() == d->tid; };
				if (std::equal(args_first, args_last, ctor->arg_types.begin(), ctor->arg_types.end(), pred))
					return ctor;
			}
			return nullptr;
		}

		const detail::type_data *data = nullptr;
	};
}	 // namespace sek::detail

namespace sek_impl
{
	template<typename T>
	struct SEK_API type_factory : sek::detail::type_factory_base<T>
	{
		static void invoke() noexcept;

		static const type_factory factory_instance;

		type_factory() noexcept { invoke(); }
	};
}	 // namespace sek_impl

#define SEK_DECLARE_TYPE_2(T, name)                                                                                    \
	namespace sek::detail                                                                                              \
	{                                                                                                                  \
		template<>                                                                                                     \
		constexpr auto generate_type_name<T>() noexcept                                                                \
		{                                                                                                              \
			return detail::basic_static_string{(name)};                                                                \
		}                                                                                                              \
	}                                                                                                                  \
	template<>                                                                                                         \
	const sek_impl::type_factory<T> sek_impl::type_factory<T>::factory_instance = {};                                  \
	template<>                                                                                                         \
	void sek_impl::type_factory<T>::invoke() noexcept

#define SEK_DECLARE_TYPE_1(T) SEK_DECLARE_TYPE_2(T, #T)
#define SEK_DISPATCH_DECLARE_TYPE(...)

/** Declares a type and creates a type factory for it.
 * It is recommended to declare types that are used at runtime, since their ids will be guaranteed.
 *
 * Accepts an optional string name, if a string name is not specified, the first argument would be
 * converted to a string and used instead.
 *
 * @note Type factory is invoked on static construction.
 *
 * @example
 * ```cpp
 * // Type id will be "my_type_name"
 * SEK_DECLARE_TYPE(my_type, "my_type_name")
 * {
 *     parent<my_type_base>();
 * }
 * ```
 * @example
 * ```cpp
 * // Type id will be "other_type"
 * SEK_DECLARE_TYPE(other_type) {}
 * ``` */
#define SEK_DECLARE_TYPE(T, ...)                                                                                       \
	SEK_GET_MACRO_2(T, ##__VA_ARGS__, SEK_DECLARE_TYPE_2, SEK_DECLARE_TYPE_1)(T, ##__VA_ARGS__)
