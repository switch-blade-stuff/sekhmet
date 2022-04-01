//
// Created by switchblade on 2022-03-28.
//

#pragma once

#include "meta_containers.hpp"
#include "type_id.hpp"

namespace sek
{
	class any;

	namespace detail
	{
		template<typename>
		constexpr bool is_exported_type = false;

		struct type_data
		{
			template<typename Node>
			struct type_node_base
			{
				constexpr void link_next(const Node *&next_ptr) noexcept
				{
					next = next_ptr;
					next_ptr = static_cast<Node *>(this);
				}

				const Node *next = nullptr;
			};

			template<typename Node>
			struct type_node_iterator
			{
				constexpr type_node_iterator() noexcept = default;
				constexpr explicit type_node_iterator(const Node *node) noexcept : node(node) {}

				constexpr void move_next() noexcept { node = node->next; }

				[[nodiscard]] constexpr bool operator==(const type_node_iterator &) const noexcept = default;

				const Node *node = nullptr;
			};
			template<typename Node>
			struct type_node_view
			{
				constexpr type_node_view() noexcept = default;
				constexpr explicit type_node_view(const Node *start, std::size_t count) noexcept
					: start(start), count(count)
				{
				}

				[[nodiscard]] constexpr auto begin() const noexcept { return type_node_iterator<Node>{start}; }
				[[nodiscard]] constexpr auto end() const noexcept { return type_node_iterator<Node>{}; }

				[[nodiscard]] constexpr auto size() const noexcept { return count; }
				[[nodiscard]] constexpr auto max_size() const noexcept
				{
					return std::numeric_limits<std::size_t>::max();
				}

				const Node *start = nullptr;
				std::size_t count = 0;
			};

			enum variant_type_t
			{
				/* If the type does not have a variant, it is a "parent". */
				VARIANT_PARENT = 0,
				VARIANT_CONST = 0b1,
				VARIANT_VOLATILE = 0b10,
				VARIANT_CONST_VOLATILE = 0b11,
				VARIANTS_MAX,
			};

			template<typename T>
			struct instance
			{
				constexpr static type_data generate() noexcept;

#if defined(SEK_OS_WIN) /* Windows requires a function-getter due to inability to export static members from a DLL. */
				static type_data *get() noexcept;
#else
				constinit static type_data value;
#endif
			};

			struct handle
			{
				constexpr handle() noexcept = default;

#if defined(SEK_OS_WIN)
				template<typename T>
				constexpr explicit handle(type_selector_t<T>) noexcept : get(type_data::instance<T>::get)
				{
				}

				[[nodiscard]] constexpr bool empty() const noexcept { return get == nullptr; }

				type_data *(*get)() noexcept = nullptr;
#else
				template<typename T>
				constexpr explicit handle(type_selector_t<T>) noexcept : ptr(&type_data::instance<T>::value)
				{
				}

				[[nodiscard]] constexpr type_data *get() const noexcept { return ptr; }
				[[nodiscard]] constexpr bool empty() const noexcept { return ptr == nullptr; }

				type_data *ptr = nullptr;
#endif

				[[nodiscard]] constexpr type_data *operator->() const noexcept { return get(); }
				[[nodiscard]] constexpr type_data &operator*() const noexcept { return *get(); }

				[[nodiscard]] constexpr bool operator==(handle other) const noexcept
				{
					return get()->tid == other->tid;
				}
			};

			template<typename T>
			[[nodiscard]] constexpr static handle make_handle() noexcept
			{
				return handle{type_selector<T>};
			}
			template<typename... Ts>
			[[nodiscard]] constexpr static meta_view<handle> make_handle_array() noexcept
			{
				constexpr auto &arg_array = array_constant<handle, make_handle<Ts>()...>::value;
				return meta_view<handle>{arg_array};
			}

			template<typename T>
			[[nodiscard]] constexpr static variant_type_t get_variant_type() noexcept
			{
				if constexpr (std::is_const_v<T> && std::is_volatile_v<T>)
					return VARIANT_CONST_VOLATILE;
				else if constexpr (std::is_const_v<T>)
					return VARIANT_CONST;
				else if constexpr (std::is_volatile_v<T>)
					return VARIANT_VOLATILE;
				else
					return VARIANT_PARENT;
			}
			template<typename T>
			[[nodiscard]] constexpr static handle get_variant_parent() noexcept
			{
				if constexpr (std::is_const_v<T> || std::is_volatile_v<T>)
					return handle{type_selector<T>};
				else
					return {};
			}
			template<typename T>
			[[nodiscard]] constexpr static handle get_const_variant() noexcept
			{
				if constexpr (!std::is_const_v<T>)
					return make_handle<std::add_const_t<T>>();
				else
					return {};
			}
			template<typename T>
			[[nodiscard]] constexpr static handle get_volatile_variant() noexcept
			{
				if constexpr (!std::is_volatile_v<T>)
					return make_handle<std::add_volatile_t<T>>();
				else
					return {};
			}
			template<typename T>
			[[nodiscard]] constexpr static handle get_cv_variant() noexcept
			{
				if constexpr (!std::is_const_v<T> && !std::is_volatile_v<T>)
					return make_handle<std::add_cv_t<T>>();
				else
					return {};
			}

			struct type_dtor
			{
				template<typename>
				struct instance
				{
					constinit static type_dtor value;
				};

				template<typename T>
				constexpr explicit type_dtor(type_selector_t<T>) noexcept requires std::destructible<T>
					: proxy([](void *ptr) { std::destroy_at(static_cast<T *>(ptr)); })
				{
				}

				void invoke(void *ptr) const { proxy(ptr); }
				void operator()(void *ptr) const { invoke(ptr); }

				void (*proxy)(void *);
			};

			template<typename T>
			[[nodiscard]] constexpr static type_dtor *make_dtor() noexcept
			{
				if constexpr (std::is_destructible_v<T>)
					return &type_dtor::instance<T>::value;
				else
					return nullptr;
			}

			struct type_ctor : type_node_base<type_ctor>
			{
				template<typename T, typename... Args>
				struct instance
				{
					template<size_t I>
					constexpr static decltype(auto) extract_arg(any args[]);
					constexpr static void ctor_impl(void *ptr, size_t n, any args[]);
					constexpr static any factory_impl(size_t n, any args[]);

					constinit static type_ctor value;
				};

				type_ctor() = delete;

				template<typename T, typename... Args>
				constexpr explicit type_ctor(type_selector_t<T>, type_seq_t<Args...>) noexcept
					: type_node_base<type_ctor>(),
					  arg_types(make_handle_array<Args...>()),
					  ctor(instance<T, Args...>::ctor_impl),
					  factory(instance<T, Args...>::factory_impl)
				{
				}

				meta_view<handle> arg_types;
				void (*ctor)(void *, size_t n, any[]);
				any (*factory)(size_t n, any[]);
			};

			template<typename T, typename... Args>
			[[nodiscard]] constexpr static type_ctor *make_ctor() noexcept
			{
				return &type_ctor::instance<T, Args...>::value;
			}
			template<typename T>
			[[nodiscard]] constexpr static type_ctor *get_default_ctor() noexcept
			{
				if constexpr (std::is_default_constructible_v<T>)
					return make_ctor<T>();
				else
					return nullptr;
			}

			struct type_parent : type_node_base<type_parent>
			{
				template<typename>
				struct instance
				{
					constinit static type_parent value;
				};

				type_parent() = delete;

				template<typename T>
				constexpr explicit type_parent(type_selector_t<T>) noexcept : type(make_handle<T>())
				{
				}

				handle type;
			};

			template<typename T>
			[[nodiscard]] constexpr static type_parent *make_parent() noexcept
			{
				return &type_parent::instance<T>::value;
			}

			struct type_attribute : type_node_base<type_attribute>
			{
				template<auto Value>
				struct instance
				{
					constexpr static type_attribute generate() noexcept
					{
						constexpr auto &data_instance = auto_constant<Value>::value;
						return type_attribute{&data_instance};
					}

					constinit static type_attribute value;
				};

				type_attribute() = delete;

				template<typename T>
				constexpr explicit type_attribute(const T *data) noexcept
					: type(make_handle<T>()), data(static_cast<const void *>(data))
				{
				}

				handle type;
				const void *data;
			};

			template<auto Value>
			[[nodiscard]] constexpr static type_attribute *make_attribute() noexcept
			{
				return &type_attribute::instance<Value>::value;
			}

			template<typename Iter>
			[[nodiscard]] constexpr const type_ctor *get_ctor(Iter first, Iter last) const noexcept
			{
				constexpr auto pred = [](handle a, type_id b) -> bool { return a->tid == b; };
				for (auto node = constructor_list; node != nullptr; node = node->next)
					if (std::equal(node->arg_types.begin(), node->arg_types.end(), first, last, pred)) return node;
				return nullptr;
			}
			template<std::ranges::forward_range R>
			[[nodiscard]] constexpr bool has_ctor(const R &arg_types) const noexcept
			{
				return get_ctor(std::ranges::begin(arg_types), std::ranges::end(arg_types)) != nullptr;
			}
			template<typename... Args>
			[[nodiscard]] constexpr bool has_ctor() const noexcept
			{
				if constexpr (sizeof...(Args) == 0)
					return has_ctor(meta_view<type_id>{});
				else
				{
					constexpr type_id arg_ids[sizeof...(Args)] = {type_id::identify<Args>()...};
					return has_ctor(arg_ids);
				}
			}

			[[nodiscard]] constexpr handle get_parent(type_id type) const noexcept
			{
				for (auto node = parent_list; node != nullptr; node = node->next)
					if (node->type->tid == type) return node->type;
				return {};
			}
			[[nodiscard]] constexpr bool has_parent(type_id type) const noexcept { return !get_parent(type).empty(); }
			template<typename T>
			[[nodiscard]] constexpr bool has_parent() const noexcept
			{
				constexpr auto id = type_id::identify<T>();
				return has_parent(id);
			}

			[[nodiscard]] constexpr const type_attribute *get_attribute(type_id type) const noexcept
			{
				for (auto node = attribute_list; node != nullptr; node = node->next)
					if (node->type->tid == type) return node;
				return nullptr;
			}
			[[nodiscard]] constexpr bool has_attribute(type_id type) const noexcept
			{
				return get_attribute(type) != nullptr;
			}
			template<typename T>
			[[nodiscard]] constexpr bool has_attribute() const noexcept
			{
				constexpr auto id = type_id::identify<T>();
				return has_attribute(id);
			}

			template<typename T, typename... Args>
			constexpr void add_ctor() noexcept
			{
				make_ctor<T, Args...>()->link_next(constructor_list);
				++constructor_count;
			}
			template<typename T>
			constexpr void add_parent() noexcept
			{
				make_parent<T>()->link_next(parent_list);
				++parent_count;
			}
			template<auto Value>
			constexpr void add_attribute() noexcept
			{
				make_attribute<Value>()->link_next(attribute_list);
				++attribute_count;
			}

			[[nodiscard]] constexpr type_node_view<type_ctor> get_ctor_view() const noexcept
			{
				return type_node_view<type_ctor>{constructor_list, constructor_count};
			}
			[[nodiscard]] constexpr type_node_view<type_parent> get_parent_view() const noexcept
			{
				return type_node_view<type_parent>{parent_list, parent_count};
			}
			[[nodiscard]] constexpr type_node_view<type_attribute> get_attribute_view() const noexcept
			{
				return type_node_view<type_attribute>{attribute_list, attribute_count};
			}

			type_id tid;

			std::size_t size;
			std::size_t alignment;

			variant_type_t variant_type;
			handle variants[VARIANTS_MAX];

			const type_dtor *destructor;

			const type_ctor *constructor_list;
			std::size_t constructor_count;

			const type_parent *parent_list;
			std::size_t parent_count;

			const type_attribute *attribute_list;
			std::size_t attribute_count;
		};

		template<typename T>
		class type_factory_base
		{
		private:
			[[nodiscard]] constexpr static type_data::handle data() noexcept { return type_data::make_handle<T>(); }

			template<typename U, typename... Us>
			static void parents_impl() noexcept requires std::is_base_of_v<U, T>
			{
				if (!data()->template has_parent<U>()) [[likely]]
					data()->template add_parent<U>();
				if constexpr (sizeof...(Us) != 0) parents_impl<Us...>();
			}
			template<auto V, auto... Vs>
			static void attributes_impl() noexcept
			{
				data()->template add_attribute<V>();
				if constexpr (sizeof...(Vs) != 0) attributes_impl<Vs...>();
			}

		protected:
			/** Adds the specified parents to the type.
			 * @tparam Ts Types to add as parents.
			 * @note Parent can only be added once. */
			template<typename... Ts>
			static void parents() noexcept
			{
				if constexpr (sizeof...(Ts) != 0) parents_impl<Ts...>();
			}
			/** Adds the specified attributes to the type.
			 * @tparam Values Values to add as attributes. */
			template<auto... Values>
			static void attributes() noexcept
			{
				attributes_impl<Values...>();
			}
			/** Adds a constructor to the type.
			 * @tparam Args Arguments of the constructor.
			 * @note If the type is default-constructible, the default constructor will be implicitly added.
			 * @note In order to pass arguments by reference, use a pointer or a reference wrapper type (ex. `std::reference_wrapper`). */
			template<typename... Args>
			static void constructor() noexcept requires std::constructible_from<T, Args...> && std::is_destructible_v<T>
			{
				if (!data()->template has_ctor<Args...>()) [[likely]]
					data()->template add_ctor<T, Args...>();
			}
		};

		template<typename T>
		constexpr type_data type_data::instance<T>::generate() noexcept
		{
			return {
				.tid = type_id::identify<T>(),
				.size = sizeof(T),
				.alignment = alignof(T),
				.variant_type = get_variant_type<T>(),
				.variants = {get_variant_parent<T>(), get_const_variant<T>(), get_volatile_variant<T>(), get_cv_variant<T>()},
				.destructor = make_dtor<T>(),
				.constructor_list = get_default_ctor<T>(),
				.constructor_count = std::is_default_constructible_v<T> ? 1 : 0,
				.parent_list = nullptr,
				.parent_count = 0,
				.attribute_list = nullptr,
				.attribute_count = 0,
			};
		}
#if defined(SEK_OS_WIN)
		template<typename T>
		type_data *type_data::instance<T>::get() noexcept
		{
			constinit static type_data result = generate();
			return &result;
		}
#else
		template<typename T>
		constinit type_data type_data::instance<T>::value = generate();
#endif

		template<typename T>
		constinit type_data::type_dtor type_data::type_dtor::instance<T>::value = type_dtor{type_selector<T>};
		template<typename T, typename... Args>
		constinit type_data::type_ctor type_data::type_ctor::instance<T, Args...>::value =
			type_ctor{type_selector<T>, type_seq<Args...>};
		template<typename T>
		constinit type_data::type_parent type_data::type_parent::instance<T>::value = type_parent{type_selector<T>};
		template<auto V>
		constinit type_data::type_attribute type_data::type_attribute::instance<V>::value = generate();
	}	 // namespace detail
}	 // namespace sek