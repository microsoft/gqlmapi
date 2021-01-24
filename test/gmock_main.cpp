// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Normally this should just link with `GTest::gmock_main`, but on Windows using shared libs (DLLs),
// that gets compiled along with with GTest using `GTEST_CREATE_SHARED_LIBRARY`, which means it
// expects to export the UnitTest singleton accessor rather than pulling it in from `GTest::gtest`.
// The `RUN_ALL_TESTS()` macro doesn't find any of the tests which are built into the executable
// because they import from `GTest::gtest` using `GTEST_LINKED_AS_SHARED_LIBRARY`. The workaround
// here is to define our own `main` entrypoint in the executable instead of relying on
// `GTest::gtest_main` or `GTest::gmock_main`. This works on any platform no matter how we link to
// the GTest/GMock libraries.
int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}