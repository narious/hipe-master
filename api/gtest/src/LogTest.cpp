#include "LogTest.h"

TEST(LogTest, Initiatation)
{
    Log log("Michael", "testlog.txt");
    EXPECT_EQ(1, 1);
}