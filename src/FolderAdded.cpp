// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

#include "FolderObject.h"

namespace graphql::mapi {

FolderAdded::FolderAdded(int index, const std::shared_ptr<Folder>& added)
	: m_index { index }
	, m_added { added }
{
}

int FolderAdded::getIndex() const
{
	return m_index;
}

std::shared_ptr<object::Folder> FolderAdded::getAdded() const
{
	return std::make_shared<object::Folder>(m_added);
}

} // namespace graphql::mapi