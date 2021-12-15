// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

#include "ItemObject.h"

namespace graphql::mapi {

ItemAdded::ItemAdded(int index, const std::shared_ptr<Item>& added)
	: m_index { index }
	, m_added { added }
{
}

int ItemAdded::getIndex() const
{
	return m_index;
}

std::shared_ptr<object::Item> ItemAdded::getAdded() const
{
	return std::make_shared<object::Item>(m_added);
}

} // namespace graphql::mapi