// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

IntValue::IntValue(int value)
	: m_value { value }
{
}

int IntValue::getValue() const
{
	return m_value;
}

} // namespace graphql::mapi