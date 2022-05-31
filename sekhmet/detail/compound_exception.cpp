//
// Created by switchblade on 31/05/22.
//

#include "compound_exception.hpp"

namespace sek
{
	void compound_exception::push(std::exception_ptr ptr) { exceptions.push_back(ptr); }
	std::string compound_exception::message() const
	{
		std::string result;
		for (auto &ptr : exceptions)
		{
			try
			{
				std::rethrow_exception(ptr);
			}
			catch (std::exception &e)
			{
				result.append(e.what()).append(1, '\n');
			}
			catch (...)
			{
				result.append("Unknown exception type\n");
			}
		}
		return result;
	}
}	 // namespace sek