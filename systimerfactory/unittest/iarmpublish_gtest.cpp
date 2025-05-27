#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iarmpublish.h"
#include "iarmpublish.cpp"

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::DoAll;

extern "C" {
    IARM_Result_t IARM_Bus_Init(const char*);
    IARM_Result_t IARM_Bus_Connect();
    IARM_Result_t IARM_Bus_IsConnected(const char*, int*);
    IARM_Result_t IARM_Bus_BroadcastEvent(const char*, int, void*, int);
}

// Mock class
class MockIarmBus {
public:
    MOCK_METHOD(IARM_Result_t, IARM_Bus_Init, (const char*));
    MOCK_METHOD(IARM_Result_t, IARM_Bus_Connect, ());
    MOCK_METHOD(IARM_Result_t, IARM_Bus_IsConnected, (const char*, int*));
    MOCK_METHOD(IARM_Result_t, IARM_Bus_BroadcastEvent, (const char*, int, void*, int));
};

MockIarmBus* g_mock = nullptr;

// C-style function hooks
extern "C" {
    IARM_Result_t IARM_Bus_Init(const char* name) {
        return g_mock->IARM_Bus_Init(name);
    }

    IARM_Result_t IARM_Bus_Connect() {
        return g_mock->IARM_Bus_Connect();
    }

    IARM_Result_t IARM_Bus_IsConnected(const char* name, int* isRegistered) {
        return g_mock->IARM_Bus_IsConnected(name, isRegistered);
    }

    IARM_Result_t IARM_Bus_BroadcastEvent(const char* owner, int event, void* data, int len) {
        return g_mock->IARM_Bus_BroadcastEvent(owner, event, data, len);
    }
}

// Test fixture
class IarmPublishTest : public ::testing::Test {
protected:
    MockIarmBus mock;

    void SetUp() override {
        g_mock = &mock;
    }

    void TearDown() override {
        g_mock = nullptr;
    }
};

TEST_F(IarmPublishTest, Constructor_DoesNotInitIfConnected) {
    int reg = 1;
    EXPECT_CALL(mock, IARM_Bus_IsConnected(StrEq("TestPublisher"), _))
        .WillOnce(DoAll(SetArgPointee<1>(reg), Return(IARM_RESULT_SUCCESS)));

    IarmPublish publisher("TestPublisher");
}

TEST_F(IarmPublishTest, Constructor_CallsInitAndConnectIfNotConnected) {
    int reg = 0;
    EXPECT_CALL(mock, IARM_Bus_IsConnected(StrEq("TestPublisher"), _))
        .WillOnce(DoAll(SetArgPointee<1>(reg), Return(IARM_RESULT_IPCCORE_FAIL)));

    EXPECT_CALL(mock, IARM_Bus_Init(StrEq("TestPublisher"))).Times(1);
    EXPECT_CALL(mock, IARM_Bus_Connect()).Times(1);

    IarmPublish publisher("TestPublisher");
}

TEST_F(IarmPublishTest, Publish_CallsBroadcastEventWithCorrectArgs) {
    int reg = 1;
    EXPECT_CALL(mock, IARM_Bus_IsConnected(StrEq("TestPublisher"), _))
        .WillOnce(DoAll(SetArgPointee<1>(reg), Return(IARM_RESULT_SUCCESS)));

    EXPECT_CALL(mock, IARM_Bus_BroadcastEvent(StrEq("TestPublisher"), 100, _, sizeof(TimerMsg)))
        .Times(1);

    IarmPublish publisher("TestPublisher");

    TimerMsg msg;
    msg.event = 100;
    msg.quality = 50;
    std::strncpy(msg.message, "Hello", sizeof(msg.message));
    msg.message[sizeof(msg.message) - 1] = '\0';  // Ensure null-termination

    publisher.publish(100, &msg);
}
int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
