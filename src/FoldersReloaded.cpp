// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

FoldersReloaded::FoldersReloaded(const std::vector<std::shared_ptr<Folder>>& reloaded)
	: m_reloaded { reloaded }
{
}

service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> FoldersReloaded::getReloaded(
	service::FieldParams&& params) const
{
	std::vector<std::shared_ptr<object::Folder>> result(m_reloaded.size());

	std::transform(m_reloaded.cbegin(),
		m_reloaded.cend(),
		result.begin(),
		[](const std::shared_ptr<Folder>& item) noexcept {
			return std::static_pointer_cast<object::Folder>(item);
		});

	return result;
}

} // namespace graphql::mapi