// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "DateTime.h"

namespace graphql::mapi {

DateTimeValue::DateTimeValue(const FILETIME& value)
	: m_value{ value }
{
}

service::FieldResult<response::Value> DateTimeValue::getValue(service::FieldParams&& params) const
{
	return { response::Value { convert::datetime::to_string(m_value) } };
}

} // namespace graphql::mapi