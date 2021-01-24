// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

ItemUpdated::ItemUpdated(response::IntType index, const std::shared_ptr<Item>& updated)
	: m_index { index }
	, m_updated { updated }
{
}

service::FieldResult<response::IntType> ItemUpdated::getIndex(service::FieldParams&& params) const
{
	return m_index;
}

service::FieldResult<std::shared_ptr<object::Item>> ItemUpdated::getUpdated(
	service::FieldParams&& params) const
{
	return std::static_pointer_cast<object::Item>(m_updated);
}

} // namespace graphql::mapi