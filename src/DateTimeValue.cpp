// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateTime.h"
#include "Types.h"

namespace graphql::mapi {

DateTimeValue::DateTimeValue(const FILETIME& value)
	: m_value { value }
{
}

response::Value DateTimeValue::getValue() const
{
	return response::Value { convert::datetime::to_string(m_value) };
}

} // namespace graphql::mapi