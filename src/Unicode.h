// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <string>
#include <string_view>

namespace convert::utf8 {

std::string to_utf8(std::wstring_view source);
std::wstring to_utf16(std::string_view source);

} // namespace convert::utf8
