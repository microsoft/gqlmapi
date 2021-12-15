// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

#include "FolderObject.h"

namespace graphql::mapi {

FoldersReloaded::FoldersReloaded(const std::vector<std::shared_ptr<Folder>>& reloaded)
	: m_reloaded { reloaded }
{
}

std::vector<std::shared_ptr<object::Folder>> FoldersReloaded::getReloaded() const
{
	std::vector<std::shared_ptr<object::Folder>> result(m_reloaded.size());

	std::transform(m_reloaded.cbegin(),
		m_reloaded.cend(),
		result.begin(),
		[](const std::shared_ptr<Folder>& folder) noexcept {
			return std::make_shared<object::Folder>(folder);
		});

	return result;
}

} // namespace graphql::mapi