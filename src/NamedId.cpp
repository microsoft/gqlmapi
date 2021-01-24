// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Guid.h"

namespace graphql::mapi {

NamedId::NamedId(const MAPINAMEID& named)
	: m_propset{ *named.lpguid }
	, m_named{ [](const MAPINAMEID& source)
	{
		switch (source.ulKind)
		{
			case MNID_ID:
				return std::static_pointer_cast<service::Object>(std::make_shared<IntId>(source.Kind.lID));

			case MNID_STRING:
				return std::static_pointer_cast<service::Object>(std::make_shared<StringId>(source.Kind.lpwstrName));

			default:
				// Name the condition so CFRt will include this in the error message.
				constexpr bool Unknown_ulKind = false;
				CFRt(Unknown_ulKind);
		}
	}(named) }
{
}

service::FieldResult<response::Value> NamedId::getPropset(service::FieldParams&& params) const
{
	return { response::Value { convert::guid::to_string(m_propset) } };
}

service::FieldResult<std::shared_ptr<service::Object>> NamedId::getId(service::FieldParams&& params) const
{
	return { m_named };
}

} // namespace graphql::mapi