// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

IntId::IntId(ULONG id)
	: m_id{ id }
{
}

service::FieldResult<response::IntType> IntId::getId(service::FieldParams&& params) const
{
	return { static_cast<response::IntType>(m_id) };
}

} // namespace graphql::mapi