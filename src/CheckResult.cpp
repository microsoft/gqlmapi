// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CheckResult.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

[[noreturn]] void ThrowHrError(HRESULT hr, PCSTR condition, PCSTR file, size_t line)
{
	std::ostringstream message;

	message << "FAILED(" << std::hex << hr << ") at " << file << ":" << std::dec << line
		<< " expected: SUCCEEDED(" << condition << ")";

	std::string report{ message.str() };

	std::cerr << report << std::endl;

	throw std::runtime_error(report);
}

[[noreturn]] void ThrowBoolFalse(PCSTR condition, PCSTR file, size_t line)
{
	std::ostringstream message;

	message << "FALSE at " << file << ":" << std::dec << line
		<< " expected: " << condition;

	std::string report{ message.str() };

	std::cerr << report << std::endl;

	throw std::runtime_error(report);
}
