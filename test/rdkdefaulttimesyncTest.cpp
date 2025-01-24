/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "rdkdefaulttimesync.h"

// Test fixture for RdkDefaultTimeSync class
class RdkDefaultTimeSyncTest : public ::testing::Test {
protected:
    RdkDefaultTimeSync timeSync;
};

// Test case for tokenize function
TEST_F(RdkDefaultTimeSyncTest, TokenizeTest) {
    std::string input = "key1=value1\nkey2=value2\nkey3=value3\n";
    std::map<std::string, std::string> expected = {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"}
    };

    std::map<std::string, std::string> result = timeSync.tokenize(input, "=");

    EXPECT_EQ(result, expected);
}

// Test case for buildtime function
TEST_F(RdkDefaultTimeSyncTest, BuildTimeTest) {
    std::string expected = "2022-01-01 12:00:00";
    std::string result = timeSync.buildtime(2022, 1, 1, 12, 0, 0);
    EXPECT_EQ(result, expected);
}

// Test case for getTime function
TEST_F(RdkDefaultTimeSyncTest, GetTimeTest) {
    std::string expected = "2022-01-01 12:00:00";
    std::string result = timeSync.getTime();
    EXPECT_EQ(result, expected);
}

// Test case for updateTime function
TEST_F(RdkDefaultTimeSyncTest, UpdateTimeTest) {
    // TODO: Add test case for updateTime function
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
