// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

SubFoldersSubscription::SubFoldersSubscription(
	std::vector<std::shared_ptr<object::FolderChange>>&& subFolders)
	: m_subFolders { std::move(subFolders) }
{
}

std::vector<std::shared_ptr<object::ItemChange>> SubFoldersSubscription::getItems(
	ObjectId&& folderIdArg) const
{
	return {};
}

std::vector<std::shared_ptr<object::FolderChange>> SubFoldersSubscription::getSubFolders(
	ObjectId&& parentFolderIdArg) const
{
	return m_subFolders;
}

std::vector<std::shared_ptr<object::FolderChange>> SubFoldersSubscription::getRootFolders(
	response::IdType&& storeIdArg) const
{
	return {};
}

} // namespace graphql::mapi