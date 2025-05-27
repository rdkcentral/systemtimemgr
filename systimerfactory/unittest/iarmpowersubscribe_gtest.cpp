#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iarmpowersubscriber.h"
#include "iarmpowersubscriber.cpp"
#include "iarmsubscribe.cpp"

extern "C" {

    typedef int IARM_Result_t;

    IARM_Result_t IARM_Bus_IsConnected(const char* busName, int* reg) {
        *reg = 0;
        return -1;  // IARM_RESULT_INVALID_PARAM assumed in headers
    }
    IARM_Result_t IARM_Bus_Init(const char* busName) {
        return 0;  // IARM_RESULT_SUCCESS assumed in headers
    }
    IARM_Result_t IARM_Bus_Connect() {
        return 0;
    }
    IARM_Result_t IARM_Bus_RegisterEventHandler(const char* ownerName, const char* eventName, IARM_EventHandler_t handler) {
        return 0;
    }
}

using ::testing::_;

namespace {
    bool testHandlerInvoked = false;

    void testPowerHandler(void* data) {
        if (!data) return;
        std::string* status = static_cast<std::string*>(data);
        if (*status == "DEEP_SLEEP_ON" || *status == "DEEP_SLEEP_OFF") {
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

TEST_F(IarmPowerSubscriberTest, SubscribeSuccess) {
    IarmPowerSubscriber subscriber("TestSub");
    IarmSubscriber::pInstance = &subscriber;

    bool subscribed = subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);
    EXPECT_TRUE(subscribed);
}

TEST_F(IarmPowerSubscriberTest, SubscribeWrongEvent) {
    IarmPowerSubscriber subscriber("TestSub");
    IarmSubscriber::pInstance = &subscriber;

    bool subscribed = subscriber.subscribe("WRONG_EVENT", testPowerHandler);
    EXPECT_FALSE(subscribed);
}

TEST_F(IarmPowerSubscriberTest, PowerEventHandler_DeepSleepOn) {
    IarmPowerSubscriber subscriber("TestSub");
    IarmSubscriber::pInstance = &subscriber;

    subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData = {};
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_OFF;

    IarmPowerSubscriber::powereventHandler(nullptr, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData, sizeof(eventData));
    EXPECT_TRUE(testHandlerInvoked);
}

TEST_F(IarmPowerSubscriberTest, PowerEventHandler_DeepSleepOff) {
    IarmPowerSubscriber subscriber("TestSub");
    IarmSubscriber::pInstance = &subscriber;

    subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData = {};
    eventData.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_ON;

    IarmPowerSubscriber::powereventHandler(nullptr, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData, sizeof(eventData));
    EXPECT_TRUE(testHandlerInvoked);
}

TEST_F(IarmPowerSubscriberTest, PowerEventHandler_EventIdMismatch) {
    IarmPowerSubscriber subscriber("TestSub");
    IarmSubscriber::pInstance = &subscriber;

    subscriber.subscribe(POWER_CHANGE_MSG, testPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData = {};
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_OFF;

    IarmPowerSubscriber::powereventHandler(nullptr, 1234, &eventData, sizeof(eventData));
    EXPECT_FALSE(testHandlerInvoked);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
