// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "Guid.h"

using namespace convert::guid;

constexpr GUID c_testGuid{ 0x12345678, 0x90ab, 0xcdef, { 0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f } };
constexpr auto c_testString = "12345678-90AB-CDEF-0819-2A3B4C5D6E7F";

TEST(ConvertGuid, ToString)
{
	const auto actual = to_string(c_testGuid);

	EXPECT_EQ(c_testString, actual) << "should convert to the expected string";
}

TEST(ConvertGuid, FromString)
{
	const auto actual = from_string(c_testString);

	EXPECT_EQ(c_testGuid, actual) << "should convert to the expected GUID";
}
