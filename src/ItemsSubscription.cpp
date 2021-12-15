// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

ItemsSubscription::ItemsSubscription(std::vector<std::shared_ptr<object::ItemChange>>&& items)
	: m_items { std::move(items) }
{
}

std::vector<std::shared_ptr<object::ItemChange>> ItemsSubscription::getItems(ObjectId&& folderIdArg) const
{
	return m_items;
}

std::vector<std::shared_ptr<object::FolderChange>> ItemsSubscription::getSubFolders(
	ObjectId&& parentFolderIdArg) const
{
	return {};
}

std::vector<std::shared_ptr<object::FolderChange>> ItemsSubscription::getRootFolders(
	response::IdType&& storeIdArg) const
{
	return {};
}

} // namespace graphql::mapi