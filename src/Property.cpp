// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"

#include "BinaryValueObject.h"
#include "BoolValueObject.h"
#include "DateTimeValueObject.h"
#include "GuidValueObject.h"
#include "IntIdObject.h"
#include "IntValueObject.h"
#include "NamedIdObject.h"
#include "PropIdObject.h"
#include "PropValueObject.h"
#include "StringValueObject.h"

namespace graphql::mapi {

Property::Property(const id_variant& id, mapi_ptr<SPropValue>&& value)
	: m_id { std::visit(
		[](const auto& id) -> std::shared_ptr<object::PropId> {
			using T = std::decay_t<decltype(id)>;

			if constexpr (std::is_same_v<T, ULONG>)
			{
				return std::make_shared<object::PropId>(
					std::make_shared<object::IntId>(std::make_shared<IntId>(id)));
			}
			else if constexpr (std::is_same_v<T, MAPINAMEID>)
			{
				return std::make_shared<object::PropId>(
					std::make_shared<object::NamedId>(std::make_shared<NamedId>(id)));
			}
		},
		id) }
	, m_value { std::move(value) }
{
}

std::shared_ptr<object::PropId> Property::getId() const
{
	return m_id;
}

std::shared_ptr<object::PropValue> Property::getValue() const
{
	std::shared_ptr<object::PropValue> result;

	switch (PROP_TYPE(m_value->ulPropTag))
	{
		case PT_I2:
			result = std::make_shared<object::PropValue>(std::make_shared<object::IntValue>(
				std::make_shared<IntValue>(static_cast<int>(m_value->Value.i))));
			break;

		case PT_LONG:
			result = std::make_shared<object::PropValue>(std::make_shared<object::IntValue>(
				std::make_shared<IntValue>(static_cast<int>(m_value->Value.l))));
			break;

		case PT_I8:
			result = std::make_shared<object::PropValue>(std::make_shared<object::IntValue>(
				std::make_shared<IntValue>(static_cast<int>(m_value->Value.li.QuadPart))));
			break;

		case PT_BOOLEAN:
			result = std::make_shared<object::PropValue>(std::make_shared<object::BoolValue>(
				std::make_shared<BoolValue>(!!m_value->Value.b)));
			break;

		case PT_STRING8:
			result = std::make_shared<object::PropValue>(std::make_shared<object::StringValue>(
				std::make_shared<StringValue>(std::string { m_value->Value.lpszA })));
			break;

		case PT_UNICODE:
			result = std::make_shared<object::PropValue>(std::make_shared<object::StringValue>(
				std::make_shared<StringValue>(m_value->Value.lpszW)));
			break;

		case PT_CLSID:
			result = std::make_shared<object::PropValue>(std::make_shared<object::GuidValue>(
				std::make_shared<GuidValue>(*m_value->Value.lpguid)));
			break;

		case PT_SYSTIME:
			result = std::make_shared<object::PropValue>(std::make_shared<object::DateTimeValue>(
				std::make_shared<DateTimeValue>(m_value->Value.ft)));
			break;

		case PT_BINARY:
			result = std::make_shared<object::PropValue>(std::make_shared<object::BinaryValue>(
				std::make_shared<BinaryValue>(m_value->Value.bin)));
			break;

		default:
			// NYI
			break;
	}

	return result;
}

} // namespace graphql::mapi