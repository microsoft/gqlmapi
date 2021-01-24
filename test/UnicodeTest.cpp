// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "Unicode.h"

using namespace convert::utf8;

using namespace std::literals;

constexpr auto c_testUtf8 = "Here's a simple test string."sv;
constexpr auto c_testUtf16 = L"Here's a simple test string."sv;

TEST(ConvertUnicode, ToUtf8)
{
	const auto actual = to_utf8(c_testUtf16);

	EXPECT_EQ(c_testUtf8, actual) << "should convert to the expected string";
}

TEST(ConvertUnicode, ToUtf16)
{
	const auto actual = to_utf16(c_testUtf8);

	EXPECT_EQ(c_testUtf16, actual) << "should convert to the expected string";
}
