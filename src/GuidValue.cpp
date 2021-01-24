// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Guid.h"

namespace graphql::mapi {

GuidValue::GuidValue(const GUID& value)
	: m_value{ value }
{
}

service::FieldResult<response::Value> GuidValue::getValue(service::FieldParams&& params) const
{
	return { response::Value { convert::guid::to_string(m_value) } };
}

} // namespace graphql::mapi