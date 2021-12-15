// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

namespace graphql::mapi {

GuidValue::GuidValue(const GUID& value)
	: m_value { value }
{
}

response::Value GuidValue::getValue() const
{
	return response::Value { convert::guid::to_string(m_value) };
}

} // namespace graphql::mapi