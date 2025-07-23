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

#include "gtest/gtest.h"
#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include "rdkdefaulttimesync.h"
#include "rdkdefaulttimesync.cpp"

using namespace testing;



TEST(RdkDefaultTimeSyncTest, getTimeBuildTimeTest) {
    // Arrange
    // Create a version.txt file with a known build time
    RdkDefaultTimeSync rdkDefaultTimeSync;
    std::ofstream verfile("/version.txt");
    verfile << "BUILD_TIME=\"2022-01-01 00:00:00\"\n";
    verfile.close();

    // Act
    long long result = rdkDefaultTimeSync.getTime();

    // Assert
    // The expected time is the Unix timestamp for 2022-01-01 00:00:00 UTC
    long long expected = 1640995200;
    EXPECT_EQ(result, expected);
}

TEST(RdkDefaultTimeSyncTest, updateTimeWithZero) {
    // Arrange
    RdkDefaultTimeSync rdkDefaultTimeSync("/tmp/clock.txt");
    // Act
     rdkDefaultTimeSync.updateTime(0);
    //ASSERT_ANY_THROW(rdkDefaultTimeSync.updateTime());
    // read the time from the file /tmp/clock.txt and convert it to nano seconds long long int
    std::ifstream file("/tmp/clock.txt");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }
    std::string timeStr;
    std::getline(file, timeStr);
    file.close();
    long long int data = std::stoll(timeStr);
    ASSERT_EQ(rdkDefaultTimeSync.getTime(), data);
}

TEST(RdkDefaultTimeSyncTest, updateTimeWithNonZero) {
    // Arrange
    RdkDefaultTimeSync rdkDefaultTimeSync("/tmp/clock.txt");
    // Create a version.txt file with a known build time
    // Act
    //ASSERT_ANY_THROW(rdkDefaultTimeSync.updateTime());
    rdkDefaultTimeSync.updateTime(1640995200);
    // read the time from the file /tmp/clock.txt and convert it to nano seconds long long int
    std::ifstream file("/tmp/clock.txt");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }
    std::string timeStr;
    std::getline(file, timeStr);
    file.close();
    long long int data = std::stoll(timeStr);
    EXPECT_EQ(rdkDefaultTimeSync.getTime(), data);
}

TEST(RdkDefaultTimeSyncTest, updateTimeWitDefaultFile) {
    RdkDefaultTimeSync rdkDefaultTimeSync;
    rdkDefaultTimeSync.updateTime(1640995200);
    ASSERT_NE(access("/opt/secure/clock.txt", F_OK), -1);
}

TEST(RdkDefaultTimeSyncTest, updateTimeWithExistingTime) {
    RdkDefaultTimeSync rdkDefaultTimeSync;
    time_t C_TIME = time(nullptr);
    rdkDefaultTimeSync.updateTime(0);
    time_t updatedTime = rdkDefaultTimeSync.getTime();
    time_t expectedTime = updatedTime + 10 * 60;
    EXPECT_GT(expectedTime, C_TIME);
}
TEST(RdkDefaultTimeSyncTest, updateTimeWithOlderTimeIncrementsBy10Minutes) {
    // Arrange
    RdkDefaultTimeSync rdkDefaultTimeSync("/tmp/clock.txt");
    // Step 1: set a large time
    rdkDefaultTimeSync.updateTime(1640995200); // Jan 1, 2022 00:00:00
    long long afterFirst = rdkDefaultTimeSync.getTime();

    // Step 2: update with an older time (smaller value)
    rdkDefaultTimeSync.updateTime(1640990000); // Dec 31, 2021 22:33:20

    // Step 3: check that current time is incremented by 10*60 (600) from afterFirst
    long long afterSecond = rdkDefaultTimeSync.getTime();
    EXPECT_EQ(afterSecond, afterFirst + 600);
}
