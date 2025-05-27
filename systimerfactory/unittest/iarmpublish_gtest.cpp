#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iarmpublish.h"
#include "iarmpublish.cpp"

// Mocking IARM Bus functions
class MockIARM_Bus {
public:
    MOCK_METHOD(IARM_Result_t, IARM_Bus_Init, (const char*), ());
    MOCK_METHOD(IARM_Result_t, IARM_Bus_Connect, (), ());
    MOCK_METHOD(IARM_Result_t, IARM_Bus_IsConnected, (const char*, int*), ());
    MOCK_METHOD(IARM_Result_t, IARM_Bus_BroadcastEvent, (const char*, IARM_EventId_t, void*, size_t), ());
};

// Global mock pointer
MockIARM_Bus* g_mockIARM = nullptr;

// Wrappers that redirect to the mock
extern "C" {
    IARM_Result_t IARM_Bus_Init(const char* name) {
        return g_mockIARM->IARM_Bus_Init(name);
    }

    IARM_Result_t IARM_Bus_Connect(void) {
        return g_mockIARM->IARM_Bus_Connect();
    }

    IARM_Result_t IARM_Bus_IsConnected(const char* name, int* isRegistered) {
        return g_mockIARM->IARM_Bus_IsConnected(name, isRegistered);
    }

    IARM_Result_t IARM_Bus_BroadcastEvent(const char* ownerName, IARM_EventId_t eventId, void* data, size_t len) {
        return g_mockIARM->IARM_Bus_BroadcastEvent(ownerName, eventId, data, len);
    }
}

// Fixture
class IarmPublishTest : public ::testing::Test {
protected:
    MockIARM_Bus mock;

    void SetUp() override {
        g_mockIARM = &mock;
    }

    void TearDown() override {
        g_mockIARM = nullptr;
    }
};

// Test case when IARM_Bus_IsConnected returns success
TEST_F(IarmPublishTest, Constructor_DoesNotInitIfConnected) {
    int reg = 1;
    EXPECT_CALL(mock, IARM_Bus_IsConnected("TestPublisher", testing::_))
        .WillOnce(DoAll(testing::SetArgPointee<1>(reg), testing::Return(IARM_RESULT_SUCCESS)));

    // Should not call Init or Connect
    EXPECT_CALL(mock, IARM_Bus_Init).Times(0);
    EXPECT_CALL(mock, IARM_Bus_Connect).Times(0);

    IarmPublish pub("TestPublisher");
}

// Test case when IARM_Bus_IsConnected fails
TEST_F(IarmPublishTest, Constructor_CallsInitAndConnectIfNotConnected) {
    int reg = 0;
    EXPECT_CALL(mock, IARM_Bus_IsConnected("TestPublisher", testing::_))
        .WillOnce(DoAll(testing::SetArgPointee<1>(reg), testing::Return(IARM_RESULT_IPCCORE_FAIL)));

    EXPECT_CALL(mock, IARM_Bus_Init("TestPublisher"))
        .WillOnce(testing::Return(IARM_RESULT_SUCCESS));
    EXPECT_CALL(mock, IARM_Bus_Connect())
        .WillOnce(testing::Return(IARM_RESULT_SUCCESS));

    IarmPublish pub("TestPublisher");
}

// Test publish method
TEST_F(IarmPublishTest, Publish_CallsBroadcastEventWithCorrectArgs) {
    IarmPublish pub("TestPublisher");

    TimerMsg msg;
    msg.event = 100;
    msg.quality = 50;
    strcpy(msg.message, "Hello");

    EXPECT_CALL(mock, IARM_Bus_BroadcastEvent("TestPublisher", 100, testing::_, sizeof(TimerMsg)))
        .WillOnce(testing::Return(IARM_RESULT_SUCCESS));

    pub.publish(100, &msg);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
