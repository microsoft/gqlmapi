// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

ItemRemoved::ItemRemoved(response::IntType index, const response::IdType& removed)
	: m_index { index }
	, m_removed { removed }
{
}

service::FieldResult<response::IntType> ItemRemoved::getIndex(service::FieldParams&& params) const
{
	return m_index;
}

service::FieldResult<response::IdType> ItemRemoved::getRemoved(service::FieldParams&& params) const
{
	return m_removed;
}

} // namespace graphql::mapi