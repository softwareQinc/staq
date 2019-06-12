/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <cassert>
#include <iostream>
#include <rang/rang.hpp>
#include <string>

namespace synthewareQ {

class diagnostic_builder;
class diagnostic_engine;

enum class diagnostic_levels {
	ignore = 0,
	note,
	warning,
	error,
};
/*! \brief A builder for diagnostics.
 *
 * An object that encapsulates a diagnostic.  The diagnostic may take
 * additional parameters and is finally issued at the end of its
 * life time.
 */
class diagnostic_builder {
public:
	/*! \brief Constructs a diagnostic builder.
	 *
	 * \param diag Diagnostic engine
	 * \param level Severity level
	 * \param message Diagnostic message
	 */
	explicit diagnostic_builder(diagnostic_engine& diag, diagnostic_levels level,
	                            std::string const& location, std::string const& message)
	    : diag_(diag)
	    , level_(level)
	    , location_(location)
	    , message_(message)
	{}

	/*! \brief Destructs the diagnostic builder and issues the diagnostic. */
	inline ~diagnostic_builder();

	diagnostic_engine& diag_;
	diagnostic_levels level_;
	std::string location_;
	std::string message_;
};

/*! \brief A diagnostic engine. */
class diagnostic_engine {
public:
	/*! \brief Creates a diagnostic builder.
	 *
	 * \param level Severity level
	 * \param message Diagnostic message
	 */
	virtual diagnostic_builder report(diagnostic_levels level, std::string const& location,
	                                  std::string const& message)
	{
		return diagnostic_builder(*this, level, location, message);
	}

	/*! \brief Emits a diagnostic message.
	 *
	 * \param level Severity level
	 * \param message Diagnostic message
	 */
	virtual inline void emit(diagnostic_builder const& diagnostic) const
	{
		using rang::style;

		if (!diagnostic.location_.empty()) {
			switch (diagnostic.level_) {
			case diagnostic_levels::ignore:
				break;

			case diagnostic_levels::note:
				std::cout << style::bold << diagnostic.location_ << ": "
				          << style::reset;
				break;

			case diagnostic_levels::warning:
			case diagnostic_levels::error:
				std::cerr << style::bold << diagnostic.location_ << ": "
				          << style::reset;
				break;

			default:
				assert(false);
			}
		}
		emit(diagnostic.level_, diagnostic.message_);
	}

	/*! \brief Emits a diagnostic message.
	 *
	 * \param level Severity level
	 * \param message Diagnostic message
	 */
	virtual inline void emit(diagnostic_levels level, const std::string& message) const
	{
		using rang::fg;
		using rang::fgB;
		using rang::style;

		switch (level) {
		case diagnostic_levels::ignore:
			break;

		case diagnostic_levels::note:
			++num_notes;
			std::cout << style::bold << fg::gray << "[note] " << fg::reset << message
			          << style::reset << '\n';
			break;

		case diagnostic_levels::warning:
			++num_warnings;
			std::cerr << style::bold << fgB::magenta << "[warning] " << fg::reset
			          << message << style::reset << '\n';
			break;

		case diagnostic_levels::error:
			++num_errors;
			std::cerr << style::bold << fgB::red << "[error] " << fg::reset << message
			          << style::reset << '\n';
			break;

		default:
			assert(false);
		}
	}

public:
	mutable unsigned num_notes = 0;
	mutable unsigned num_warnings = 0;
	mutable unsigned num_errors = 0;
};

diagnostic_builder::~diagnostic_builder()
{
	diag_.emit(*this);
}

class error_diagnostic_engine : public diagnostic_engine {
public:
 	virtual void emit(diagnostic_levels level, const std::string& message) const override
 	{
      using rang::fg;
      using rang::fgB;
      using rang::style;

      switch (level) {
      case diagnostic_levels::error:
			++num_errors;
			std::cerr << style::bold << fgB::red << "[error] " << fg::reset << message
			          << style::reset << '\n';
			break;
      default:
        break;
 	  }
    }
};

} // namespace synthewareQ
