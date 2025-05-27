#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)

#include <gtest/gtest.h>
#include <iostream>
#include "power_controller.h"
#include "ipowercontrollersubscriber.h"
#include "ipowercontrollersubscriber.cpp"


using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

extern "C" {
    // Mockable declarations for linking
    IARM_Result_t IARM_Bus_IsConnected(const char* name, int* isRegistered);
    IARM_Result_t IARM_Bus_Init(const char* name);
    IARM_Result_t IARM_Bus_Connect();
    bool IARM_Bus_RegisterEventHandler(const char* ownerName, int eventId, void (*handler)(const char*, int, void*, size_t));
}

// === Mock Implementations ===
IARM_Result_t IARM_Bus_IsConnected(const char* name, int* isRegistered) {
    *isRegistered = 0; // Simulate not connected
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Init(const char* name) {
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Connect() {
    return IARM_RESULT_SUCCESS;
}

bool IARM_Bus_RegisterEventHandler(const char* ownerName, int eventId, void (*handler)(const char*, int, void*, size_t)) {
    return true;
}

// === Global test state ===
std::string g_lastPowerStatus;
void MockPowerHandler(void* status) {
    g_lastPowerStatus = *(static_cast<std::string*>(status));
}

// === Google Test Fixture ===
class IarmPowerSubscriberTest : public ::testing::Test {
protected:
    IarmPowerSubscriber* subscriber;

    void SetUp() override {
        g_lastPowerStatus.clear();
        subscriber = new IarmPowerSubscriber("TestModule");
        IarmSubscriber::pInstance = subscriber;  // Singleton instance for dynamic_cast
    }

    void TearDown() override {
        delete subscriber;
        IarmSubscriber::pInstance = nullptr;
    }
};

// === Test Cases ===

TEST_F(IarmPowerSubscriberTest, SubscribeRegistersEventHandler) {
    EXPECT_TRUE(subscriber->subscribe(POWER_CHANGE_MSG, MockPowerHandler));
}

TEST_F(IarmPowerSubscriberTest, PowerEventTriggersHandlerForDeepSleep) {
    subscriber->subscribe(POWER_CHANGE_MSG, MockPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData;
    memset(&eventData, 0, sizeof(eventData));
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_OFF;

    IarmPowerSubscriber::powereventHandler("Test", IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData, sizeof(eventData));

    EXPECT_EQ(g_lastPowerStatus, "DEEP_SLEEP_ON");
}

TEST_F(IarmPowerSubscriberTest, PowerEventTriggersHandlerForWakeUp) {
    subscriber->subscribe(POWER_CHANGE_MSG, MockPowerHandler);

    IARM_Bus_PWRMgr_EventData_t eventData;
    memset(&eventData, 0, sizeof(eventData));
    eventData.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
    eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_ON;

    IarmPowerSubscriber::powereventHandler("Test", IARM_BUS_PWRMGR_EVENT_MODECHANGED, &eventData, sizeof(eventData));

    EXPECT_EQ(g_lastPowerStatus, "DEEP_SLEEP_OFF");
}

TEST_F(IarmPowerSubscriberTest, NonMatchingEventDoesNotTriggerHandler) {
    subscriber->subscribe(POWER_CHANGE_MSG, MockPowerHandler);

    g_lastPowerStatus = "SHOULD_NOT_CHANGE";
    IarmPowerSubscriber::powereventHandler("Test", 9999, nullptr, 0);

    EXPECT_EQ(g_lastPowerStatus, "SHOULD_NOT_CHANGE");
}
