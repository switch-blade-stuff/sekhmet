/*
 * Created by switchblade on 21/06/22
 */

#include "archive_error.hpp"

namespace sek::serialization
{
	archive_error::~archive_error() = default;

	class archive_category_t : public std::error_category
	{
	public:
		constexpr archive_category_t() noexcept = default;
		~archive_category_t() override = default;

		[[nodiscard]] const char *name() const noexcept override { return "archive"; }
		[[nodiscard]] std::string message(int err) const override
		{
			switch (static_cast<archive_errc>(err))
			{
				case archive_errc::INVALID_TYPE: return "Invalid type";
				case archive_errc::UNEXPECTED_END: return "Unexpected EOF";
				default: return "Unknown error";
			}
		}
	};

	const std::error_category &archive_category() noexcept
	{
		constinit static const archive_category_t instance;
		return instance;
	}
}	 // namespace sek::serialization