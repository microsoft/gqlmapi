// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

ItemRemoved::ItemRemoved(int index, const response::IdType& removed)
	: m_index { index }
	, m_removed { removed }
{
}

int ItemRemoved::getIndex() const
{
	return m_index;
}

const response::IdType& ItemRemoved::getRemoved() const
{
	return m_removed;
}

} // namespace graphql::mapi