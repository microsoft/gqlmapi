// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

StringId::StringId(PCWSTR name)
	: m_name{ name }
{
}

service::FieldResult<response::StringType> StringId::getName(service::FieldParams&& params) const
{
	return { convert::utf8::to_utf8(m_name) };
}

} // namespace graphql::mapi