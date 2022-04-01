//
// Created by switchblade on 2022-02-13.
//

#pragma once

#include <utility>

#include "../../detail/any.hpp"
#include "node.hpp"

namespace sek::adt
{
	/** @brief Attribute used to set type's serialization & deserialization functions. */
	struct serializable_as_attribute
	{
		struct proxy_t
		{
			typedef void (*serialize_func_t)(node &, sek::any);
			typedef void (*deserialize_func_t)(const node &, sek::any);

			serialize_func_t serialize_func = nullptr;
			deserialize_func_t deserialize_func = nullptr;
		};

		/** Invokes the serialize proxy function. */
		constexpr void serialize(node &n, sek::any r) const { proxy.serialize_func(n, std::move(r)); }
		/** Invokes the deserialize proxy function. */
		constexpr void deserialize(const node &n, sek::any r) const { proxy.deserialize_func(n, std::move(r)); }

		/** Proxy used to store serialization functions. */
		const proxy_t &proxy;
	};

	namespace detail
	{
		template<typename T>
		struct serialize_as_proxy
		{
			constexpr static serializable_as_attribute::proxy_t bind() noexcept
			{
				constexpr auto serialize = [](node &n, sek::any r)
				{
					auto &value = *r.template as<const T>();
					n.template set<T>(value);
				};
				constexpr auto deserialize = [](const node &n, sek::any r)
				{
					auto &value = *r.template as<T>();
					n.template get<T>(value);
				};

				return {serialize, deserialize};
			}

			constinit static const serializable_as_attribute::proxy_t instance;
		};

		template<typename T>
		constinit const serializable_as_attribute::proxy_t serialize_as_proxy<T>::instance = bind();
	}	 // namespace detail

	/** Variable used to create an instance of `serializable_as_attribute` for
	 * default type serialization (uses `node::get<T>` & `node::set<T>.`). */
	template<typename T>
	constexpr inline serializable_as_attribute serializable_as = {detail::serialize_as_proxy<T>::instance};

	inline void node::get(sek::any &value) const
	{
		if (auto attr = value.type().get_attribute<serializable_as_attribute>(); attr == nullptr) [[unlikely]]
			throw sek::bad_type_exception("Missing `serializable_as_attribute` attribute");
		else
			attr->deserialize(*this, value.as_ref());
	}
	inline node &node::set(const sek::any &value)
	{
		if (auto attr = value.type().get_attribute<serializable_as_attribute>(); attr == nullptr) [[unlikely]]
			throw sek::bad_type_exception("Missing `serializable_as_attribute` attribute");
		else
			attr->serialize(*this, value.as_ref());
		return *this;
	}
}	 // namespace sek::adt