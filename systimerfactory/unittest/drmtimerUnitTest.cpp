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

#include "Client_Mock.h"
//print the Debug Lines on the console remove rdklogger dependency
#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)

#include "drmtimersrc.h"
#include "drmtimersrc.cpp"

using namespace testing;

#define DRM_TEST_FILE "/opt/DRMTEST.txt"

struct drmTestFixture : public testing::Test 
{   
    TestMock *Test =  new TestMock();
    Json::Value value;
    void SetUp() override
    {
        globalTestClient = Test;
        printf("%p\n",globalTestClient);
    }
    void TearDown() override
    {
        delete Test;
        globalTestClient=nullptr;
        printf("%p\n",globalTestClient);
    }   
};


TEST_F(drmTestFixture, getTimeSecTestwithINactive){
    //ARRANGE
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    value["activationstatus"]="iNActive";
    // ACT 
    EXPECT_CALL(*Test, getReturnValue(_,_)).Times(1).WillOnce(Return(value));
    //ASSERT
    ASSERT_EQ(TestDrmTime.getTimeSec(), 0); 
}



TEST_F(drmTestFixture, getTimeSecTestwithActiveWithoutSuccess){
   
    //ARRANGE
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    //ASSERT
    ASSERT_EQ(TestDrmTime.getTimeSec(), 0); 
}

TEST_F(drmTestFixture, getTimeSecTestwithActiveWithSuccess){
   
    //ARRANGE
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    value["activationstatus"]="Active";
    value["success"]=true;  
    // ACT 
    EXPECT_CALL(*Test, getReturnValue(_,_)).Times(2).WillOnce(Return(value));
    //ASSERT
    ASSERT_EQ(TestDrmTime.getTimeSec(), 0); 
}
TEST_F(drmTestFixture, getTimeSecTestwithActiveWithSecureTime){
    //ARRANGE
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    long long time = 1234567890;
    value["activationstatus"] = "Active";
    value["success"] = true;
    value["securetime"] = std::to_string(time);
    std::cout << "Value is " << value << std::endl;
    struct timespec times;  
    timespec_get(&times, TIME_UTC);
    // ACT 
    EXPECT_CALL(*Test, getReturnValue(_,_)).Times(2).WillOnce(Return(value)).WillOnce(Return(value));
    //ASSERT
    ASSERT_EQ(TestDrmTime.getTimeSec(), time) << "Time returned is not as expected";
    ASSERT_NE(access(DRM_TEST_FILE, F_OK), -1) << "File has not been created";
    int fd = open(DRM_TEST_FILE, O_RDONLY);
    ASSERT_NE(fd, -1)<< "File not found; may be access error modify testcase";
    struct stat sb;
    ASSERT_NE(fstat(fd, &sb), -1) << "fstat failed; may be access error modify testcase";
    ASSERT_GT(sb.st_atime, times.tv_nsec) << "Access time not updated";

}

TEST_F(drmTestFixture, isReferenceTest){
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    ASSERT_FALSE(TestDrmTime.isreference());
}

TEST_F(drmTestFixture, checkTimeTestTrue){
    value["activationstatus"] = "Active";
    value["success"] = true;
    value["securetime"] = "123456789";
    EXPECT_CALL(*Test, getReturnValue(_,_)).Times(2).WillOnce(Return(value)).WillOnce(Return(value));
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    ASSERT_FALSE(TestDrmTime.checkTime());
}

TEST_F(drmTestFixture, isClockProviderTest){
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    ASSERT_FALSE(TestDrmTime.isclockProvider());
}

TEST(drmStaticTest, checkTimeTest){
    DrmTimeSrc TestDrmTime(DRM_TEST_FILE);
    ASSERT_TRUE(TestDrmTime.checkTime());
}


int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
