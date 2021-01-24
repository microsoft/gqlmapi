// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

BoolValue::BoolValue(response::BooleanType value)
	: m_value{ value }
{
}

service::FieldResult<response::BooleanType> BoolValue::getValue(service::FieldParams&& params) const
{
	return { static_cast<response::BooleanType>(m_value) };
}

} // namespace graphql::mapi