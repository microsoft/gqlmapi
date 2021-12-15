// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

RootFoldersSubscription::RootFoldersSubscription(
	std::vector<std::shared_ptr<object::FolderChange>>&& rootFolders)
	: m_rootFolders { std::move(rootFolders) }
{
}

std::vector<std::shared_ptr<object::ItemChange>> RootFoldersSubscription::getItems(
	ObjectId&& folderIdArg) const
{
	return {};
}

std::vector<std::shared_ptr<object::FolderChange>> RootFoldersSubscription::getSubFolders(
	ObjectId&& parentFolderIdArg) const
{
	return {};
}

std::vector<std::shared_ptr<object::FolderChange>> RootFoldersSubscription::getRootFolders(
	response::IdType&& storeIdArg) const
{
	return m_rootFolders;
}

} // namespace graphql::mapi