// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

ItemsReloaded::ItemsReloaded(const std::vector<std::shared_ptr<Item>>& reloaded)
	: m_reloaded { reloaded }
{
}

service::FieldResult<std::vector<std::shared_ptr<object::Item>>> ItemsReloaded::getReloaded(
	service::FieldParams&& params) const
{
	std::vector<std::shared_ptr<object::Item>> result(m_reloaded.size());

	std::transform(m_reloaded.cbegin(),
		m_reloaded.cend(),
		result.begin(),
		[](const std::shared_ptr<Item>& item) noexcept {
			return std::static_pointer_cast<object::Item>(item);
		});

	return result;
}

} // namespace graphql::mapi