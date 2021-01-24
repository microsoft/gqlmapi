// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

ItemsSubscription::ItemsSubscription(std::vector<std::shared_ptr<service::Object>>&& items)
	: m_items { std::move(items) }
{
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> ItemsSubscription::getItems(
	service::FieldParams&& params, ObjectId&& folderIdArg) const
{
	return m_items;
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> ItemsSubscription::getSubFolders(
	service::FieldParams&& params, ObjectId&& parentFolderIdArg) const
{
	return std::vector<std::shared_ptr<service::Object>> {};
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> ItemsSubscription::getRootFolders(
	service::FieldParams&& params, response::IdType&& storeIdArg) const
{
	return std::vector<std::shared_ptr<service::Object>> {};
}

} // namespace graphql::mapi