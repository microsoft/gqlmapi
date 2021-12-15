// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Unicode.h"

namespace graphql::mapi {

BinaryValue::BinaryValue(const SBinary& value)
	: m_value { [](const SBinary& source) {
		const auto idBegin = reinterpret_cast<std::uint8_t*>(source.lpb);
		const auto idEnd = idBegin + static_cast<size_t>(source.cb);

		return response::IdType { idBegin, idEnd };
	}(value) }
{
}

const response::IdType& BinaryValue::getValue() const
{
	return m_value;
}

} // namespace graphql::mapi