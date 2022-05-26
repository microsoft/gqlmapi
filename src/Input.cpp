// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Input.h"

#include <algorithm>

using namespace graphql;

namespace convert::input {

response::IdType from_input(response::IdType&& input)
{
	return response::IdType { input.release<response::IdType::ByteData>() };
}

mapi::ObjectId from_input(mapi::ObjectId&& input)
{
	return mapi::ObjectId {
		input.storeId.release<response::IdType::ByteData>(),
		input.objectId.release<response::IdType::ByteData>(),
	};
}

mapi::PropValueInput from_input(mapi::PropValueInput&& input)
{
	if (input.bin)
	{
		input.bin = std::make_optional(
			response::IdType { input.bin->release<response::IdType::ByteData>() });
	}

	return input;
}

mapi::CreateItemInput from_input(mapi::CreateItemInput&& input)
{
	if (input.conversationId)
	{
		input.conversationId = std::make_optional(
			response::IdType { input.conversationId->release<response::IdType::ByteData>() });
	}

	return input;
}

mapi::MultipleItemsInput from_input(mapi::MultipleItemsInput&& input)
{
	for (auto& itemId : input.itemIds)
	{
		itemId = response::IdType { itemId.release<response::IdType::ByteData>() };
	}

	return input;
}

} // namespace convert::input
