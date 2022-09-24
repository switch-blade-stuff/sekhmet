/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include <memory>

#include "../serialization/ubjson.hpp"
#include "../type_info.hpp"
#include "assets.hpp"

namespace sek::engine
{
	class resource_cache;

	namespace attributes
	{
		class serializable_resource;
	}

	/** @brief Exception thrown by the resource system on runtime errors. */
	class SEK_API resource_error : public std::runtime_error
	{
	public:
		resource_error() : std::runtime_error("Unknown asset resource") {}
		explicit resource_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit resource_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit resource_error(const char *msg) : std::runtime_error(msg) {}
		~resource_error() override;
	};

	/** @brief Service used to load resources and manage resource cache. */
	class resource_cache : public service<access_guard<resource_cache, std::recursive_mutex>>
	{
		friend class access_guard<resource_cache, std::recursive_mutex>;

	protected:
		using attribute_t = attributes::serializable_resource;

		struct metadata_t
		{
			constexpr metadata_t() noexcept = default;
			explicit metadata_t(const asset_handle &);

			type_info type;			 /* Type info of the resource's type. */
			const attribute_t *attr; /* Pointer to the cached attribute. */
		};

		struct cache_entry
		{
			explicit cache_entry(const asset_handle &asset) : metadata(asset) {}

			metadata_t metadata;	  /* Metadata of the resource. */
			std::weak_ptr<void> data; /* Weak pointer to the resource's data. */
		};

		static any load_anonymous(metadata_t metadata, asset_source &src);

	public:
		/** Loads a resource form an asset source, completely bypassing the cache.
		 * @param type Type of the resource.
		 * @param src Asset source containing asset's data.
		 * @return `any` containing the loaded resource.
		 * @throw resource_error If `type` is not a valid resource type. */
		static SEK_API any load_anonymous(type_info type, asset_source &src);
		/** Loads a resource form an asset, completely bypassing the cache.
		 * @param asset Asset to load the resource from.
		 * @return `any` containing the loaded resource.
		 * @throw resource_error If the asset is not a valid resource. */
		static SEK_API any load_anonymous(const asset_handle &asset);

	protected:
		constexpr resource_cache() = default;

	public:
		/** Loads a resource from an asset.
		 * @param asset Asset to load the resource from.
		 * @param copy If set to true, the resource will be copied from the cache.
		 * @return Shared pointer to the resource or a null pointer if the asset handle is empty.
		 * @throw resource_error If the asset is not a valid resource. */
		std::shared_ptr<void> load(const asset_handle &asset, bool copy = false)
		{
			return load_impl(asset, copy).first;
		}
		/** @copydoc load
		 * @note Casts the resource to type `T` using it's type info. */
		template<typename T>
		std::shared_ptr<T> load(const asset_handle &asset, bool copy = false)
		{
			auto [ptr, metadata] = load_impl(asset, copy);
			return cast_impl<T>(std::move(ptr), metadata);
		}

		/** Loads a resource from an asset using it's name.
		 * @param name Name of the resource's asset.
		 * @param copy If set to true, the resource will be copied from the cache.
		 * @return Shared pointer to the resource or a null pointer if such asset does not exist.
		 * @throw resource_error If the asset exists but is not a valid resource. */
		std::shared_ptr<void> load(std::string_view name, bool copy = false) { return load_impl(name, copy).first; }
		/** @copydoc load
		 * @note Casts the resource to type `T` using it's type info. */
		template<typename T>
		std::shared_ptr<T> load(std::string_view name, bool copy = false)
		{
			auto [ptr, metadata] = load_impl(name, copy);
			return cast_impl<T>(std::move(ptr), metadata);
		}
		/** Loads a resource from an asset using it's UUID.
		 * @param id Id of the resource's asset.
		 * @param copy If set to true, the resource will be copied from the cache.
		 * @return Shared pointer to the resource or a null pointer if such asset does not exist.
		 * @throw resource_error If the asset exists but is not a valid resource. */
		std::shared_ptr<void> load(uuid id, bool copy = false) { return load_impl(id, copy).first; }
		/** @copydoc load
		 * @note Casts the resource to type `T` using it's type info. */
		template<typename T>
		std::shared_ptr<T> load(uuid id, bool copy = false)
		{
			auto [ptr, metadata] = load_impl(id, copy);
			return cast_impl<T>(std::move(ptr), metadata);
		}

		/** Removes cache entries of all resources of the specified type.
		 * @return Amount of resources removed.
		 * @note Cache entries of relevant types should be cleared on plugin unload to avoid stale references. */
		SEK_API std::size_t clear(type_info type);
		/** Removes cache entry for resource with the specified id. */
		SEK_API void clear(uuid id);
		/** Removes all cache entries. */
		SEK_API void clear();

	protected:
		SEK_API std::pair<std::shared_ptr<void>, metadata_t *> load_impl(const asset_handle &asset, bool copy);
		SEK_API std::pair<std::shared_ptr<void>, metadata_t *> load_impl(std::string_view name, bool copy);
		SEK_API std::pair<std::shared_ptr<void>, metadata_t *> load_impl(uuid id, bool copy);

		template<typename T>
		std::shared_ptr<T> cast_impl(std::shared_ptr<void> &&ptr, metadata_t *metadata)
		{
			if (ptr) [[likely]]
			{
				auto a = metadata->attr->m_forward_ref(ptr.get());
				return std::shared_ptr<T>{std::move(ptr), std::addressof(a.template cast<std::add_lvalue_reference_t<T>>())};
			}
			return {};
		}

		dense_map<uuid, cache_entry> m_cache;				  /* Cache of resource instances. */
		dense_map<std::string_view, dense_set<uuid>> m_types; /* Cache of resource types. */
	};

	namespace attributes
	{
		/** @brief Attribute used to designate a type as a runtime-serializable resource. */
		class serializable_resource
		{
			friend class engine::resource_cache;

			using default_input = typename serialization::json_object::read_frame;
			using default_output = typename serialization::json_object::write_frame;

		public:
			/** @brief Initializes resource attribute for type `T` with input archive `Input` and output archive `Output`.
			 * @note Output archive type is only relevant in editor. */
			template<typename T, serialization::input_archive I = default_input, serialization::output_archive O = default_output>
			constexpr serializable_resource(type_selector_t<T>, type_selector_t<I>, type_selector_t<O>) noexcept
			{
				static_assert(serialization::in_place_deserializable<T, I, resource_cache &>);
				static_assert(serialization::serializable<T, O>);

				m_instantiate = +[]() { return std::static_pointer_cast<void>(std::make_shared<T>()); };
				m_copy = +[](const void *ptr)
				{
					auto &t_ref = *static_cast<const T *>(ptr);
					return std::static_pointer_cast<void>(std::make_shared<T>(t_ref));
				};

				m_forward_ref = +[](void *ptr) -> any_ref
				{
					auto &t_ref = *static_cast<T *>(ptr);
					return any_ref{forward_any(t_ref)};
				};

				m_deserialize = +[](void *ptr, asset_source &src, resource_cache &cache, float &state)
				{
					using reader_t = typename I::reader_type;
					using char_t = typename I::char_type;
					using traits_t = std::char_traits<char_t>;

					typename reader_t::callback_info callbacks = {
						.getn = +[](void *p, char_t *dst, std::size_t n) -> std::size_t
						{
							auto src = static_cast<asset_source *>(p);
							return src->read(dst, n * sizeof(char_t));
						},
						.bump = +[](void *p, std::size_t n) -> std::size_t
						{
							auto src = static_cast<asset_source *>(p);
							const auto bytes = static_cast<std::int64_t>(n * sizeof(char_t));
							const auto pos = src->tell();
							return static_cast<std::size_t>(src->seek(bytes, asset_source::cur) - pos);
						},
						.tell = +[](void *p) -> std::size_t
						{
							auto src = static_cast<asset_source *>(p);
							return static_cast<std::size_t>(src->tell());
						},
						.peek = +[](void *p) -> typename traits_t::int_type
						{
							char_t c;
							auto src = static_cast<asset_source *>(p);
							auto total = src->read(&c, sizeof(char_t));

							/* Seek back to the original position. */
							src->seek(-static_cast<std::int64_t>(total), asset_source::cur);

							if (total == sizeof(char_t)) [[likely]]
								return traits_t::to_int_type(c);
							return traits_t::eof();
						},
						.take = +[](void *p) -> typename traits_t::int_type
						{
							auto src = static_cast<asset_source *>(p);
							if (char_t c; src->read(&c, sizeof(char_t)) == sizeof(char_t)) [[likely]]
								return traits_t::to_int_type(c);
							return traits_t::eof();
						},
					};

					auto archive = I{reader_t{&callbacks, &src}};
					archive.read(*static_cast<T *>(ptr), cache, (state = 0.0f));
				};

#ifdef SEK_EDITOR /* Serialization is editor-only. */
				m_serialize = +[](void *ptr, system::native_file &dst)
				{
					using writer_t = typename O::writer_type;
					auto archive = O{writer_t{dst}};
					archive << *static_cast<T *>(ptr);
				};
#endif
			}
			/** @brief Initializes resource attribute for type `T` with default (UBJson) input & output archives. */
			template<typename T>
			constexpr explicit serializable_resource(type_selector_t<T> s) noexcept : serializable_resource(s, {}, {})
			{
			}

		private:
			constexpr void deserialize(void *ptr, asset_source &src, resource_cache &cache, float &state) const
			{
				m_deserialize(ptr, src, cache, state);
			}
			constexpr void deserialize(void *ptr, asset_source &src, float &state) const
			{
				deserialize(ptr, src, *resource_cache::instance()->access(), state);
			}

			std::shared_ptr<void> (*m_instantiate)();
			std::shared_ptr<void> (*m_copy)(const void *);

			any_ref (*m_forward_ref)(void *);

			void (*m_deserialize)(void *, asset_source &, resource_cache &, float &);

#ifdef SEK_EDITOR
			void (*m_serialize)(void *, system::native_file &);
#endif
		};

		/** Helper function used to create an instance of `serializable_resource` attribute for type `T`.
		 * @tparam T Type to designate as a serializable resource.
		 * @tparam I Input archive type to use for deserialization (UBJson by default).
		 * @tparam O Output archive type to use for serialization (UBJson by default). */
		template<typename T,
				 serialization::input_archive<T> I = serialization::ubj::input_archive,
				 serialization::output_archive<T> O = serialization::ubj::output_archive>
		[[nodiscard]] constexpr serializable_resource make_serializable_resource() noexcept
		{
			return serializable_resource{type_selector<T>, type_selector<I>, type_selector<O>};
		}
	}	 // namespace attributes
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::access_guard<sek::engine::resource_cache, std::recursive_mutex>>;