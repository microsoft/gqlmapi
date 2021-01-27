// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#ifdef GQLMAPI_DLLEXPORTS
	#define GQLMAPI_EXPORT __declspec(dllexport)
#else // !GQLMAPI_DLLEXPORTS
	#define GQLMAPI_EXPORT
#endif // !GQLMAPI_DLLEXPORTS
// clang-format on

#include "Types.h"

namespace graphql::mapi {

GQLMAPI_EXPORT std::shared_ptr<Operations> GetService(bool useDefaultProfile) noexcept
{
	auto session = std::make_shared<Session>(useDefaultProfile);
	auto query = std::make_shared<Query>(session);
	auto mutation = std::make_shared<Mutation>(query);
	auto subscription = std::make_shared<Subscription>(query);
	auto service = std::make_shared<Operations>(query, mutation, subscription);

	subscription->setService(service);
	return service;
}

} // namespace graphql::mapi