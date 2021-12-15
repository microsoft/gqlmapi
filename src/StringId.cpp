// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

StringId::StringId(PCWSTR name)
	: m_name { convert::utf8::to_utf8(name) }
{
}

const std::string& StringId::getName() const
{
	return m_name;
}

} // namespace graphql::mapi