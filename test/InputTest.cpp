// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "Input.h"

#include <string_view>

using namespace graphql;
using namespace convert::input;

response::IdType getByteData() noexcept
{
	using namespace std::literals;

	const auto fakeIdString = "fakeId"sv;
	response::IdType result(fakeIdString.size());

	std::copy(fakeIdString.begin(), fakeIdString.end(), result.begin());

	return result;
}

response::IdType getOpaqueString() noexcept
{
	return response::IdType { getByteData().release<response::IdType::OpaqueString>() };
}

TEST(ConvertInput, FromIdTypeByteData)
{
	const auto expected = getByteData();
	const auto actual = convert::input::from_input(getByteData());

	EXPECT_FALSE(expected.empty()) << "getByteData should return a non-empty response::IdType";
	EXPECT_FALSE(actual.empty()) << "from_input should return a non-empty response::IdType";
	EXPECT_EQ(expected, actual) << "from_input should match expected";
	EXPECT_EQ(expected.get<response::IdType::ByteData>(), actual.get<response::IdType::ByteData>())
		<< "underlying storage should match";
}

TEST(ConvertInput, FromIdTypeOpaqueString)
{
	const auto expected = getByteData();
	auto actual = convert::input::from_input(getOpaqueString());

	EXPECT_FALSE(expected.empty()) << "getByteData should return a non-empty response::IdType";
	EXPECT_FALSE(actual.empty()) << "from_input should return a non-empty response::IdType";
	EXPECT_EQ(expected, actual) << "from_input should match expected";
	EXPECT_EQ(expected.get<response::IdType::ByteData>(),
		actual.release<response::IdType::ByteData>())
		<< "converted value should match";
}
