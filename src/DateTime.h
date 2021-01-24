// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <windows.h>

#include <string>

namespace convert::datetime {

std::string to_string(const FILETIME& ft);
FILETIME from_string(const std::string& value);

} // namespace convert::datetime
