#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iarmpowersubscriber.h"
#include "iarmpowersubscriber.cpp"

using ::testing::_;
using ::testing::Return;

// Mocks
extern "C" {
    MOCK_FUNC2(IARM_Bus_IsConnected, IARM_Result_t(const char*, int*));
    MOCK_FUNC1(IARM_Bus_Init, IARM_Result_t(const char*));
    MOCK_FUNC0(IARM_Bus_Connect, IARM_Result_t(void));
    MOCK_FUNC3(IARM_Bus_RegisterEventHandler, IARM_Result_t(const char*, const char*, IARM_EventHandler_t));
}

namespace {
    bool testHandlerInvoked = false;

    void testPowerHandler(void* data) {
        std::string* status = static_cast<std::string*>(data);
        if (status && (*status == "DEEP_SLEEP_ON" || *status == "DEEP_SLEEP_OFF")) {
            testHandlerInvoked = true;
        }
    }
}

class IarmPowerSubscriberTest : public ::testing::Test {
protected:
    void SetUp() override {
        testHandlerInvoked = false;
    }

    void TearDown() override {
        IarmSubscriber::pInstance = nullptr;
    }
};

// Positive case for constructor and subscribe
TEST_F(IarmPowerSubscriberTest, SubscribeSuccess) {
    EXPECT_CALL(::testing::MockFunction<IARM_Result_t(const char*, int*)>(), IARM_Bus_IsConnected(_, _))
        .WillOnce([](const char*, int* reg) {
            *reg = 0;
            return IARM_RESULT_INVALID_PARAM;
        });

    EXPECT_CALL(::testing::MockFunction<IARM_Result_t(const char*)>(), IARM_Bus_Init(_))
        .WillOnce(Return(IARM_RESULT_SUCCESS));

    EXPECT_CALL(::testing::MockFunction<IARM_Result_t(void)>(), IARM_Bus_Connect())
        .WillOnce(Return(IARM_RESULT_SUCCESS));

    EXPECT_CALL(::testing::MockFunction<IARM_Result_t(const char*, const char*, IARM_EventHandler_t)>(),
                IARM_Bus_RegisterEventHandler(_, _, _))
        .WillOnce(Return(IARM_RESULT_SUCCESS));

    IarmPowerSubscriber subscriber("TestSub");
    bool subscribed = subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);
    EXPECT_TRUE(subscribed);
}

// Negative case: wrong event
TEST_F(IarmPowerSubscriberTest, SubscribeWrongEvent) {
    IarmPowerSubscriber subscriber("TestSub");
    bool subscribed = subscriber.subscribe("WRONG_EVENT", testPowerHandler);
    EXPECT_FALSE(subscribed);
}

// Test power event handler for DEEP_SLEEP_ON
TEST_F(IarmPowerSubscriberTest, PowerEventHandler_DeepSleepOn) {
    IarmPowerSubscriber subscriber("TestSub");
    subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData = {};
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_OFF;

    IarmPowerSubscriber::powereventHandler(nullptr, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData, sizeof(eventData));
    EXPECT_TRUE(testHandlerInvoked);
}

// Test power event handler for DEEP_SLEEP_OFF
TEST_F(IarmPowerSubscriberTest, PowerEventHandler_DeepSleepOff) {
    IarmPowerSubscriber subscriber("TestSub");
    subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData = {};
    eventData.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_ON;

    IarmPowerSubscriber::powereventHandler(nullptr, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData, sizeof(eventData));
    EXPECT_TRUE(testHandlerInvoked);
}

// Negative test: event ID mismatch
TEST_F(IarmPowerSubscriberTest, PowerEventHandler_EventIdMismatch) {
    IarmPowerSubscriber subscriber("TestSub");
    subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData = {};
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_OFF;

    IarmPowerSubscriber::powereventHandler(nullptr, 1234, &eventData, sizeof(eventData));
    EXPECT_FALSE(testHandlerInvoked);
}

