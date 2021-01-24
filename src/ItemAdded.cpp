// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

ItemAdded::ItemAdded(response::IntType index, const std::shared_ptr<Item>& added)
	: m_index { index }
	, m_added { added }
{
}

service::FieldResult<response::IntType> ItemAdded::getIndex(service::FieldParams&& params) const
{
	return m_index;
}

service::FieldResult<std::shared_ptr<object::Item>> ItemAdded::getAdded(
	service::FieldParams&& params) const
{
	return std::static_pointer_cast<object::Item>(m_added);
}

} // namespace graphql::mapi