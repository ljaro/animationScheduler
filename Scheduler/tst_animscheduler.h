#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

class Animation {};
class Scheduler {};


TEST(AnimScheduler, test1)
{
    EXPECT_EQ(1, 1);
    ASSERT_THAT(0, Eq(0));
}
