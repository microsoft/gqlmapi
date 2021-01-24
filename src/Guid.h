// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <guiddef.h>

#include <string>

namespace convert::guid {

std::string to_string(const GUID& guid);
GUID from_string(const std::string& value);

} // namespace convert::guid
