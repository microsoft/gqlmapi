// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Guid.h"
#include "Types.h"

#include "IntIdObject.h"
#include "NamedPropIdObject.h"
#include "StringIdObject.h"

namespace graphql::mapi {

NamedId::NamedId(const MAPINAMEID& named)
	: m_propset { *named.lpguid }
	, m_named { [](const MAPINAMEID& source) {
		switch (source.ulKind)
		{
			case MNID_ID:
				return std::make_shared<object::NamedPropId>(
					std::make_shared<object::IntId>(std::make_shared<IntId>(source.Kind.lID)));

			case MNID_STRING:
				return std::make_shared<object::NamedPropId>(std::make_shared<object::StringId>(
					std::make_shared<StringId>(source.Kind.lpwstrName)));

			default:
				// Name the condition so CFRt will include this in the error message.
				constexpr bool Unknown_ulKind = false;
				CFRt(Unknown_ulKind);
		}
	}(named) }
{
}

response::Value NamedId::getPropset() const
{
	return response::Value { convert::guid::to_string(m_propset) };
}

std::shared_ptr<object::NamedPropId> NamedId::getId() const
{
	return m_named;
}

} // namespace graphql::mapi