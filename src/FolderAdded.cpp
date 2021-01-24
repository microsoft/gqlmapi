// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

FolderAdded::FolderAdded(response::IntType index, const std::shared_ptr<Folder>& added)
	: m_index { index }
	, m_added { added }
{
}

service::FieldResult<response::IntType> FolderAdded::getIndex(service::FieldParams&& params) const
{
	return m_index;
}

service::FieldResult<std::shared_ptr<object::Folder>> FolderAdded::getAdded(
	service::FieldParams&& params) const
{
	return std::static_pointer_cast<object::Folder>(m_added);
}

} // namespace graphql::mapi