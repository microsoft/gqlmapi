// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

BoolValue::BoolValue(bool value)
	: m_value { value }
{
}

bool BoolValue::getValue() const
{
	return m_value;
}

} // namespace graphql::mapi