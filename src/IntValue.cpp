// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

IntValue::IntValue(response::IntType value)
	: m_value{ value }
{
}

service::FieldResult<response::IntType> IntValue::getValue(service::FieldParams&& params) const
{
	return { static_cast<response::IntType>(m_value) };
}

} // namespace graphql::mapi