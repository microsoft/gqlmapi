// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

#include "FolderObject.h"

namespace graphql::mapi {

FolderUpdated::FolderUpdated(int index, const std::shared_ptr<Folder>& updated)
	: m_index { index }
	, m_updated { updated }
{
}

int FolderUpdated::getIndex() const
{
	return m_index;
}

std::shared_ptr<object::Folder> FolderUpdated::getUpdated() const
{
	return std::make_shared<object::Folder>(m_updated);
}

} // namespace graphql::mapi