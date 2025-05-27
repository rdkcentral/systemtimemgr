#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)

#include <gtest/gtest.h>
#include <iostream>
#include "power_controller.h"
#include "ipowercontrollersubscriber.h"
#include "ipowercontrollersubscriber.cpp"

// Fake PowerController state
static PowerController_PowerModeChangedCb g_callback = nullptr;
static void* g_callback_data = nullptr;
static bool g_callback_registered = false;
static bool g_init_called = false;
static bool g_term_called = false;

// --- Mock Implementations of PowerController C API ---

extern "C" {



void PowerController_Init() {
    g_init_called = true;
}

void PowerController_Term() {
    g_term_called = true;
}

uint32_t PowerController_Connect() {
    return g_callback_registered ? POWER_CONTROLLER_ERROR_NONE : POWER_CONTROLLER_ERROR_GENERAL;
}

uint32_t PowerController_RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb cb, void* data) {
    g_callback = cb;
    g_callback_data = data;
    g_callback_registered = true;
    return POWER_CONTROLLER_ERROR_NONE;
}

uint32_t PowerController_UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb cb) {
    if (cb == g_callback) {
        g_callback_registered = false;
        g_callback = nullptr;
        g_callback_data = nullptr;
        return POWER_CONTROLLER_ERROR_NONE;
    }
    return POWER_CONTROLLER_ERROR_GENERAL;
}

} // extern "C"
int TestPowerHandler(void* data) {
    g_callback_data = data;
    callback_invoked = true;

    // Optionally cast the data to expected type (e.g., PowerController_PowerState_t*)
    return 0;  // Return success
}

// Handler to test callback propagation
/*void TestPowerHandler(void* status) {
    std::string* msg = static_cast<std::string*>(status);
    std::cout << "Handler invoked with status: " << *msg << std::endl;
} */
// === Google Test Fixture ===
class IpowerControllerSubscriberTest : public ::testing::Test {
protected:
    IpowerControllerSubscriber* subscriber;

    void SetUp() override {
        g_init_called = false;
        g_term_called = false;
        g_callback_registered = false;
        g_callback = nullptr;
        g_callback_data = nullptr;
        subscriber = new IpowerControllerSubscriber("SysTimeMgr");
    }

    void TearDown() override {
        delete subscriber;
    }
};

// === Tests ===

TEST_F(IpowerControllerSubscriberTest, SubscribeAndCallbackSuccess) {
    bool subscribed = subscriber->subscribe(POWER_CHANGE_MSG, TestPowerHandler);
    EXPECT_TRUE(subscribed);
    EXPECT_TRUE(g_init_called);
    EXPECT_TRUE(g_callback_registered);

    // Simulate PowerController invoking callback
    if (g_callback) {
        g_callback(reinterpret_cast<void*>(data_pointer));
 // Should invoke TestPowerHandler
    }
}

TEST_F(IpowerControllerSubscriberTest, SubscribeFailsConnectStartsThread) {
    // Set condition where connect always fails
    g_callback_registered = false;

    bool subscribed = subscriber->subscribe(POWER_CHANGE_MSG, TestPowerHandler);
    EXPECT_TRUE(subscribed);  // Thread still starts
}

TEST_F(IpowerControllerSubscriberTest, DestructorUnregistersCallback) {
    subscriber->subscribe(POWER_CHANGE_MSG, TestPowerHandler);
    delete subscriber;
    subscriber = nullptr;

    EXPECT_FALSE(g_callback_registered);
    EXPECT_TRUE(g_term_called);
}
