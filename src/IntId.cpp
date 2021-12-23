// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

namespace graphql::mapi {

IntId::IntId(ULONG id)
	: m_id { id }
{
}

int IntId::getId() const
{
	return static_cast<int>(m_id);
}

} // namespace graphql::mapi