//
// Created by switchblade on 2022-02-13.
//

#pragma once

#include "../../detail/any.hpp"
#include "node.hpp"

namespace sek::adt::detail
{
	/** @brief Attribute used to set type's serialization & deserialization functions. */
	struct serializable_as_attribute
	{
		struct proxy_t
		{
			typedef void (*serialize_func_t)(node &, sek::detail::any_ref);
			typedef void (*deserialize_func_t)(const node &, sek::detail::any_ref);

			serialize_func_t serialize_func = nullptr;
			deserialize_func_t deserialize_func = nullptr;
		};

		/** Invokes the serialize proxy function. */
		constexpr void serialize(node &n, sek::detail::any_ref r) const { proxy.serialize_func(n, r); }
		/** Invokes the deserialize proxy function. */
		constexpr void deserialize(const node &n, sek::detail::any_ref r) const { proxy.deserialize_func(n, r); }

		/** Proxy used to store serialization functions. */
		const proxy_t &proxy;
	};

	template<typename T>
	struct serialize_as_proxy
	{
		constexpr static serializable_as_attribute::proxy_t bind() noexcept
		{
			constexpr auto serialize = [](node &n, sek::detail::any_ref r)
			{
				auto &value = r.template as<const T>();
				n.template set<T>(value);
			};
			constexpr auto deserialize = [](const node &n, sek::detail::any_ref r)
			{
				auto &value = r.template as<T>();
				n.template get<T>(value);
			};

			return {serialize, deserialize};
		}

		constinit static const serializable_as_attribute::proxy_t instance;
	};

	template<typename T>
	constinit const serializable_as_attribute::proxy_t serialize_as_proxy<T>::instance = bind();

	/** Variable used to create an instance of `serializable_as_attribute` for
	 * default type serialization (uses `node::get<T>` & `node::set<T>.`). */
	template<typename T>
	constexpr inline serializable_as_attribute serializable_as = {serialize_as_proxy<T>::instance};

	inline void node::get(sek::detail::any_ref value) const
	{
		if (auto attr = value.type().get_attribute<serializable_as_attribute>(); attr == nullptr) [[unlikely]]
			throw sek::detail::bad_type_exception("Missing `serializable_as_attribute` attribute");
		else
			attr->deserialize(*this, value);
	}
	inline node &node::set(sek::detail::any_ref value)
	{
		if (auto attr = value.type().get_attribute<serializable_as_attribute>(); attr == nullptr) [[unlikely]]
			throw sek::detail::bad_type_exception("Missing `serializable_as_attribute` attribute");
		else
			attr->serialize(*this, value);
		return *this;
	}
}	 // namespace sek::adt::detail