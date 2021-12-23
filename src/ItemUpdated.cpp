// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

#include "ItemObject.h"

namespace graphql::mapi {

ItemUpdated::ItemUpdated(int index, const std::shared_ptr<Item>& updated)
	: m_index { index }
	, m_updated { updated }
{
}

int ItemUpdated::getIndex() const
{
	return m_index;
}

std::shared_ptr<object::Item> ItemUpdated::getUpdated() const
{
	return std::make_shared<object::Item>(m_updated);
}

} // namespace graphql::mapi