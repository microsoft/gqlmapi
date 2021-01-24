// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Unicode.h"
#include "CheckResult.h"

#include <windows.h>

namespace convert::utf8 {

std::string to_utf8(std::wstring_view source)
{
	std::string result;

	if (!source.empty())
	{
		const auto cch = WideCharToMultiByte(CP_UTF8, 0, source.data(), source.size(), nullptr, 0, nullptr, nullptr);

		CFRt(cch > 0);
		result.resize(static_cast<size_t>(cch));
		CFRt(cch == WideCharToMultiByte(CP_UTF8, 0, source.data(), source.size(), result.data(), result.size(), nullptr, nullptr));
	}

	return result;
}

std::wstring to_utf16(std::string_view source)
{
	std::wstring result;

	if (!source.empty())
	{
		const auto cch = MultiByteToWideChar(CP_UTF8, 0, source.data(), source.size(), nullptr, 0);

		CFRt(cch > 0);
		result.resize(static_cast<size_t>(cch));
		CFRt(cch == MultiByteToWideChar(CP_UTF8, 0, source.data(), source.size(), result.data(), result.size()));
	}

	return result;
}

} // namespace convert::utf8
