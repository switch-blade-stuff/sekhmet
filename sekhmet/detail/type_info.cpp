/*
 * Created by switchblade on 16/05/22
 */

#include "type_info.hpp"

#include "dense_map.hpp"
#include <fmt/format.h>
#include <shared_mutex>

namespace sek
{
	namespace detail
	{
		void assert_mutable_any(const any &a, std::string_view name)
		{
			if (a.is_const()) [[unlikely]]
				throw any_const_error(fmt::format("Cannot bind const `any` to a non-const type \"{}\"", name));
		}
	}	 // namespace detail

	struct type_db
	{
		static type_db &instance()
		{
			static type_db value;
			return value;
		}

		mutable std::shared_mutex mtx;
		dense_map<std::string_view, detail::type_handle> types;
	};


	type_info_error::~type_info_error() = default;
	any_type_error::~any_type_error() = default;
	any_const_error::~any_const_error() = default;
	invalid_member_error::~invalid_member_error() = default;

	type_info::data_t &type_info::register_type(handle_t handle) noexcept
	{
		auto &db = type_db::instance();
		std::lock_guard<std::shared_mutex> l(db.mtx);
		return *db.types.try_emplace(handle->name, handle).first->second;
	}
	type_info type_info::get(std::string_view name) noexcept
	{
		auto &db = type_db::instance();
		std::shared_lock<std::shared_mutex> l(db.mtx);

		if (auto handle_iter = db.types.find(name); handle_iter != db.types.end()) [[likely]]
			return type_info{handle_iter->second};
		else
			return type_info{};
	}
	void type_info::reset(std::string_view name) noexcept
	{
		auto &db = type_db::instance();
		std::lock_guard<std::shared_mutex> l(db.mtx);
		db.types.erase(name);
	}

	static std::string args_type_msg(auto begin, auto end, auto &&name_get)
	{
		std::string result = "[";
		std::size_t i = 0;
		std::for_each(begin,
					  end,
					  [&i, &result, &name_get](auto &&v)
					  {
						  if (i++ != 0) [[likely]]
							  result.append(", ");
						  result.append(1, '\"').append(name_get(v)).append(1, '\"');
					  });
		result.append("]");
		return result;
	}

	any type_info::construct(std::span<any> args) const
	{
		const auto ctors = constructors();
		const auto ctor = std::ranges::find_if(ctors, [&args](auto c) { return c.signature().invocable_with(args); });
		if (ctor == ctors.end()) [[unlikely]]
		{
			throw invalid_member_error(
				fmt::format("No matching constructor taking {} found for type \"{}\"",
							args_type_msg(args.begin(), args.end(), [](auto &&a) { return a.type().name(); }),
							name()));
		}
		else
			return ctor->m_node->invoke(args);
	}
	any type_info::invoke(std::string_view name, any instance, std::span<any> args) const
	{
		const auto funcs = functions();
		const auto func = std::ranges::find_if(funcs, [&name](auto f) { return f.name() == name; });
		if (func == funcs.end()) [[unlikely]]
		{
			throw invalid_member_error(fmt::format("No matching function with name \"{}\" "
												   "found for type \"{}\"",
												   name,
												   m_data->name));
		}
		else
			return func->invoke(std::move(instance), args);
	}
	any type_info::get_attribute(std::string_view name) const noexcept
	{
		const auto attribs = attributes();
		auto iter = std::find_if(attribs.begin(), attribs.end(), [name](auto n) { return n.type().name() == name; });
		if (iter != attribs.end()) [[likely]]
			return iter->value();
		else
			return any{};
	}
	any type_info::get_attribute(type_info info) const noexcept { return get_attribute(info.name()); }

	any any::convert(std::string_view n) noexcept
	{
		if (m_info.name() == n)
			return ref();
		else
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = m_info.parents();
			auto p_iter = std::find_if(parents.begin(), parents.end(), [n](auto p) { return p.type().name() == n; });
			if (p_iter != parents.end()) [[likely]]
				return p_iter->cast(*this);

			/* Attempt to cast to an explicit conversion. */
			const auto convs = m_info.conversions();
			auto conv_iter = std::find_if(convs.begin(), convs.end(), [n](auto c) { return c.type().name() == n; });
			if (conv_iter != convs.end()) [[likely]]
				return conv_iter->convert(ref());

			/* Search up the inheritance hierarchy. */
			for (auto p : parents)
			{
				auto p_result = p.cast(*this).convert(n);
				if (!p_result.empty()) [[likely]]
					return p_result;
			}

			return {};
		}
	}
	any any::convert(type_info to_type) noexcept { return convert(to_type.name()); }
	any any::convert(std::string_view n) const noexcept
	{
		if (m_info.name() == n)
			return ref();
		else
		{
			/* Attempt to cast to an immediate parent. */
			const auto parents = m_info.parents();
			auto p_iter = std::find_if(parents.begin(), parents.end(), [n](auto p) { return p.type().name() == n; });
			if (p_iter != parents.end()) [[likely]]
				return p_iter->cast(*this);

			/* Attempt to cast to an explicit conversion. */
			const auto convs = m_info.conversions();
			auto conv_iter = std::find_if(convs.begin(), convs.end(), [n](auto c) { return c.type().name() == n; });
			if (conv_iter != convs.end()) [[likely]]
				return conv_iter->convert(ref());

			/* Search up the inheritance hierarchy. */
			for (auto p : parents)
			{
				const auto p_cast = p.cast(*this);
				auto p_result = p_cast.convert(n);
				if (!p_result.empty()) [[likely]]
					return p_result;
			}

			return {};
		}
	}
	any any::convert(type_info to_type) const noexcept { return convert(to_type.name()); }
	any any::invoke(std::string_view name, std::span<any> args) { return type().invoke(name, ref(), args); }
	any any::invoke(std::string_view name, std::span<any> args) const { return type().invoke(name, ref(), args); }
	any any_ref::convert(std::string_view n) noexcept { return value().convert(n); }
	any any_ref::convert(type_info to_type) noexcept { return value().convert(to_type); }
	any any_ref::convert(std::string_view n) const noexcept { return value().convert(n); }
	any any_ref::convert(type_info to_type) const noexcept { return value().convert(to_type); }
	any any_ref::invoke(std::string_view name, std::span<any> args) { return type().invoke(name, *this, args); }
	any any_ref::invoke(std::string_view name, std::span<any> args) const { return type().invoke(name, *this, args); }

	bool signature_info::assert_args(std::span<any> values) const
	{
		if (!invocable_with(values)) [[unlikely]]
		{
			const auto as = args();
			throw any_type_error(
				fmt::format("Invalid argument types. Expected: {}, got {}",
							args_type_msg(as.begin(), as.end(), [](auto &&t) { return t.name(); }),
							args_type_msg(values.begin(), values.end(), [](auto &&a) { return a.type().name(); })));
		}
		return true;
	}
}	 // namespace sek
