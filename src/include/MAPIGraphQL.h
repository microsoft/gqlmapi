// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "MAPIObjects.h"

namespace graphql::mapi {

std::shared_ptr<Operations> GetService(bool useDefaultProfile) noexcept;

} // namespace graphql::mapi