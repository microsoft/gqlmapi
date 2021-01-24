// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

SubFoldersSubscription::SubFoldersSubscription(std::vector<std::shared_ptr<service::Object>>&& subFolders)
	: m_subFolders { std::move(subFolders) }
{
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> SubFoldersSubscription::getItems(
	service::FieldParams&& params, ObjectId&& folderIdArg) const
{
	return std::vector<std::shared_ptr<service::Object>> {};
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> SubFoldersSubscription::getSubFolders(
	service::FieldParams&& params, ObjectId&& parentFolderIdArg) const
{
	return m_subFolders;
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> SubFoldersSubscription::getRootFolders(
	service::FieldParams&& params, response::IdType&& storeIdArg) const
{
	return std::vector<std::shared_ptr<service::Object>> {};
}

} // namespace graphql::mapi