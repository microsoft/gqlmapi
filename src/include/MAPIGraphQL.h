// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

// clang-format off
#ifdef GQLMAPI_DLLEXPORTS
	#define GQLMAPI_IMPORT __declspec(dllimport)
#else // !GQLMAPI_DLLEXPORTS
	#define GQLMAPI_IMPORT
#endif // !GQLMAPI_DLLEXPORTS
// clang-format on

#include "graphqlservice/GraphQLService.h"

namespace graphql::mapi {

GQLMAPI_IMPORT std::shared_ptr<service::Request> GetService(bool useDefaultProfile) noexcept;

} // namespace graphql::mapi