// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <windows.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

[[noreturn]] void ThrowHrError(HRESULT hr, PCSTR condition, PCSTR file, size_t line);
[[noreturn]] void ThrowBoolFalse(PCSTR condition, PCSTR file, size_t line);

#define CORt(result) \
	{ \
		const HRESULT hr = (result); \
		if (FAILED(hr)) \
		{ \
			ThrowHrError(hr, #result, __FILE__, __LINE__); \
		} \
	}

#define CFRt(result) \
	{ \
		if (!(result)) \
		{ \
			ThrowBoolFalse(#result, __FILE__, __LINE__); \
		} \
	}
