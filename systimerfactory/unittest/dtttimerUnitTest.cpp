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

#include "dtttimersrc.h"
#include "dtttimersrc.cpp"

using namespace testing;

#define DTT_TEST_FILE "/opt/DTTTEST.txt"

struct dttTestFixture : public testing::Test 
{   
    TestMock *Test =  new TestMock();
    Json::Value value;
    void SetUp() override
    {
        globalTestClient = Test;
       // printf("%p\n",globalTestClient);
    }
    void TearDown() override
    {
        delete Test;
        globalTestClient=nullptr;
        //printf("%p\n",globalTestClient);
    }   
};


TEST_F(dttTestFixture, getTimeSecTestwithNoSuccess){
    //ARRANGE
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    value["success"]=false;
    // ACT 
    EXPECT_CALL(*Test, getReturnValue("Controller.1.activate",_)).Times(1).WillOnce(Return(value));
    EXPECT_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_)).Times(1).WillOnce(Return(value));
    //ASSERT
    ASSERT_EQ(TestDttTime.getTimeSec(), 0); 
}



TEST_F(dttTestFixture, getTimeSecTestwithSuccessNoinfo){
   
    //ARRANGE
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    value["success"]=true;
    // ACT
    EXPECT_CALL(*Test, getReturnValue("Controller.1.activate",_)).Times(1).WillOnce(Return(value));
    EXPECT_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_)).Times(1).WillOnce(Return(value));
    
    //ASSERT
    ASSERT_THROW(TestDttTime.getTimeSec(), std::invalid_argument);
}

TEST_F(dttTestFixture, getTimeSecTestwithSuccessInfo){
   
    //ARRANGE
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    value["success"] = true;
    value["info"]["timeUtc"] = 1234567890;
    // ACT 
    EXPECT_CALL(*Test, getReturnValue("Controller.1.activate",_)).Times(1).WillOnce(Return(value));
    EXPECT_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_)).Times(1).WillOnce(Return(value));
    //ASSERT
    ASSERT_EQ(TestDttTime.getTimeSec(), 1234567890); 
}
TEST_F(dttTestFixture, getTimeSecTestActivateonce){
   
    //ARRANGE
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    value["success"] = true;
    value["info"]["timeUtc"] = 1234567890;
    // ACT 
    ON_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_))
    .WillByDefault([](const std::string&, const Json::Value&) -> Json::Value {
        std:cout << "Called too many times" << std::endl;
        throw std::runtime_error("Called too many times");
    });
    EXPECT_CALL(*Test, getReturnValue("Controller.1.activate",_)).Times(1).WillOnce(Return(value));
    EXPECT_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_))
    .Times(2)
    .WillOnce(Return(value));
    ASSERT_EQ(TestDttTime.getTimeSec(), 1234567890);
    ASSERT_ANY_THROW(TestDttTime.getTimeSec());

}
TEST_F(dttTestFixture, getTimeSecTestActivateonceNegetive){
   
    //ARRANGE
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    value["success"] = false;

    // ACT 
    EXPECT_CALL(*Test, getReturnValue("Controller.1.activate",_)).Times(1).WillOnce(Return(value));
    EXPECT_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_)).Times(2).WillRepeatedly(Return(value));
    ASSERT_EQ(TestDttTime.getTimeSec(), 0);
    ASSERT_EQ(TestDttTime.getTimeSec(), 0);

}


/*
TEST_F(dttTestFixture, getTimeSecTestwithActiveWithSecureTime){
    //ARRANGE
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
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
    ASSERT_EQ(TestDttTime.getTimeSec(), time) << "Time returned is not as expected";
    ASSERT_NE(access(DTT_TEST_FILE, F_OK), -1) << "File has not been created";
    int fd = open(DTT_TEST_FILE, O_RDONLY);
    ASSERT_NE(fd, -1)<< "File not found; may be access error modify testcase";
    struct stat sb;
    ASSERT_NE(fstat(fd, &sb), -1) << "fstat failed; may be access error modify testcase";
    ASSERT_GT(sb.st_atime, times.tv_nsec) << "Access time not updated";

}
*/

TEST_F(dttTestFixture, isReferenceTest){
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    ASSERT_FALSE(TestDttTime.isreference());
}

TEST_F(dttTestFixture, checkTimeTestTrue){
    value["success"] = true;
    value["info"]["timeUtc"] = 1234567890;
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    // ACT 
    EXPECT_CALL(*Test, getReturnValue("Controller.1.activate",_)).Times(1).WillOnce(Return(value));
    EXPECT_CALL(*Test, getReturnValue("org.rdk.MediaSystem.1.getBroadcastTimeInfo",_)).Times(1).WillRepeatedly(Return(value));
    
    ASSERT_FALSE(TestDttTime.checkTime());
}

// need to check this with DRM implementation

TEST_F(dttTestFixture, isClockProviderTest){
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    ASSERT_TRUE(TestDttTime.isclockProvider());
}

TEST(drmStaticTest, checkTimeTest){
    DttTimeSrc TestDttTime(DTT_TEST_FILE);
    ASSERT_TRUE(TestDttTime.checkTime());
}


int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
