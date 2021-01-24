// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

std::shared_ptr<Operations> GetService(bool useDefaultProfile) noexcept
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