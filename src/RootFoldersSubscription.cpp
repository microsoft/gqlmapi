// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

RootFoldersSubscription::RootFoldersSubscription(std::vector<std::shared_ptr<service::Object>>&& rootFolders)
	: m_rootFolders { std::move(rootFolders) }
{
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> RootFoldersSubscription::getItems(
	service::FieldParams&& params, ObjectId&& folderIdArg) const
{
	return std::vector<std::shared_ptr<service::Object>> {};
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> RootFoldersSubscription::getSubFolders(
	service::FieldParams&& params, ObjectId&& parentFolderIdArg) const
{
	return std::vector<std::shared_ptr<service::Object>> {};
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> RootFoldersSubscription::getRootFolders(
	service::FieldParams&& params, response::IdType&& storeIdArg) const
{
	return m_rootFolders;
}

} // namespace graphql::mapi