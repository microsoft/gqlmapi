// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

FolderUpdated::FolderUpdated(response::IntType index, const std::shared_ptr<Folder>& updated)
	: m_index { index }
	, m_updated { updated }
{
}

service::FieldResult<response::IntType> FolderUpdated::getIndex(service::FieldParams&& params) const
{
	return m_index;
}

service::FieldResult<std::shared_ptr<object::Folder>> FolderUpdated::getUpdated(
	service::FieldParams&& params) const
{
	return std::static_pointer_cast<object::Folder>(m_updated);
}

} // namespace graphql::mapi