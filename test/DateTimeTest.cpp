// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "DateTime.h"

using namespace convert::datetime;

constexpr SYSTEMTIME c_testTime{
	2020,	// wYear (2020)
	11,		// wMonth (November)
	0,		// wDayOfWeek (Sunday)
	15,		// wDay (15th)
	17,		// wHour (5 PM)
	15,		// wMinute (15 minutes)
	30,		// wSecond (30 seconds)
	75,		// wMilliseconds (0.075 seconds)
};

constexpr auto c_testString = "2020-11-15T17:15:30.075Z";

FILETIME ConvertUtcSystime(const SYSTEMTIME& input)
{
	FILETIME result{};

	if (!SystemTimeToFileTime(&input, &result))
	{
		throw std::exception("SystemTimeToFileTime failed!");
	}

	return result;
}

constexpr bool operator==(const FILETIME& lhs, const FILETIME& rhs) noexcept
{
	return lhs.dwHighDateTime == rhs.dwHighDateTime
		&& lhs.dwLowDateTime == rhs.dwLowDateTime;
}

TEST(ConvertDateTime, ToString)
{
	const auto actual = to_string(ConvertUtcSystime(c_testTime));

	EXPECT_EQ(c_testString, actual) << "should convert to the expected string";
}

TEST(ConvertDateTime, FromString)
{
	const auto actual = from_string(c_testString);

	EXPECT_EQ(ConvertUtcSystime(c_testTime), actual) << "should convert to the expected FILETIME";
}
