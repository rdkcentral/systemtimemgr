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
static bool callback_invoked = false;
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
    // Cast data to a struct that contains the info you need
    // Or just test that this was called
    return 0;
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

    PowerController_PowerState newState = POWER_STATE_ON;
    PowerController_PowerState oldState = POWER_STATE_OFF;
    void* user_data = reinterpret_cast<void*>(0x1234);

    g_callback = TestPowerHandler;  // Simulate internal assignment
    ASSERT_NE(g_callback, nullptr);

    g_callback(newState, oldState, user_data);
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(g_callback_data, user_data);
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
