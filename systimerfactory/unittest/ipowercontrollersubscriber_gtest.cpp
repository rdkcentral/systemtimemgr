#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)

#include <gtest/gtest.h>
#include <iostream>
#include "power_controller.h"
#include "ipowercontrollersubscriber.h"
#include "ipowercontrollersubscriber.cpp"
#include "iarmsubscribe.cpp"

//IarmSubscriber* IarmSubscriber::pInstance = nullptr;


// Fake PowerController state
static PowerController_PowerModeChangedCb g_callback = nullptr;
static void* g_callback_data = nullptr;
static bool g_callback_registered = true;
static bool callback_invoked = false;
static bool g_init_called = false;
static bool g_term_called = false;
PowerController_PowerState g_powerStateNew;
PowerController_PowerState g_powerStateOld;
void* g_userData = nullptr;

// --- Mock Implementations of PowerController C API ---

extern "C" {




void PowerController_Init() {
    g_init_called = true;
}

void PowerController_Term() {
    g_term_called = true;
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
void TestPowerHandler(PowerController_PowerState newState, PowerController_PowerState oldState, void* userData) {
    // Store values or assert for test
    g_powerStateNew = newState;
    g_powerStateOld = oldState;
    g_userData = userData;
}
int PowerModeAdapterCallback(void* data) {
    if (g_callback) {
        // Optionally fill mock data here
        g_callback(PowerController_PowerState::POWER_STATE_ON, PowerController_PowerState::POWER_STATE_OFF, data);
    }
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
        g_callback_registered = true;
        g_callback = nullptr;
        g_callback_data = nullptr;
        subscriber = new IpowerControllerSubscriber("SysTimeMgr");
    }

    void TearDown() override {
        delete subscriber;
    }
};

int retry_count = 0;

extern "C" uint32_t PowerController_Connect() {
    retry_count++;
    if (retry_count < 3) {
        return POWER_CONTROLLER_ERROR_GENERAL;
    } else {
        g_callback_registered = true;
        return POWER_CONTROLLER_ERROR_NONE;
    }
}

TEST_F(IpowerControllerSubscriberTest, ConnectEventuallySucceedsAfterRetries) {
    retry_count = 0;
    g_callback_registered = false;

    bool subscribed = subscriber->subscribe(POWER_CHANGE_MSG, PowerModeAdapterCallback);
    EXPECT_TRUE(subscribed);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Let retry thread run

    EXPECT_GE(retry_count, 3);  // Should retry at least 3 times
    EXPECT_TRUE(g_callback_registered);
}

// === Tests ===

/*TEST_F(IpowerControllerSubscriberTest, SubscribeAndCallbackSuccess) {
    bool subscribed = subscriber->subscribe(POWER_CHANGE_MSG, PowerModeAdapterCallback);
    EXPECT_TRUE(subscribed);

    PowerController_PowerState newState = POWER_STATE_ON;
    PowerController_PowerState oldState = POWER_STATE_OFF;
    void* user_data = reinterpret_cast<void*>(0x1234);

    g_callback = TestPowerHandler;  // Simulate internal assignment
    ASSERT_NE(g_callback, nullptr);

    g_callback(newState, oldState, user_data);
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(g_callback_data, user_data);
}*/
TEST_F(IpowerControllerSubscriberTest, SubscribeFailsConnectRetriesInThread) {
    g_callback_registered = false;  // Simulate Connect failure

    bool subscribed = subscriber->subscribe(POWER_CHANGE_MSG, PowerModeAdapterCallback);
    EXPECT_TRUE(subscribed);

    // Sleep briefly to let the retry thread run once or twice
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify connect did not succeed (still false)
    EXPECT_FALSE(g_callback_registered);
}

TEST_F(IpowerControllerSubscriberTest, SubscribeFailsConnectStartsThread) {
    // Set condition where connect always fails
    g_callback_registered = false;

    bool subscribed = subscriber->subscribe(POWER_CHANGE_MSG, PowerModeAdapterCallback);
    EXPECT_TRUE(subscribed);  // Thread still starts
}

TEST_F(IpowerControllerSubscriberTest, DestructorUnregistersCallback) {
    subscriber->subscribe(POWER_CHANGE_MSG, PowerModeAdapterCallback);
    delete subscriber;
    subscriber = nullptr;

    EXPECT_FALSE(g_callback_registered);  // Now correct after fixing unregister logic
    EXPECT_TRUE(g_term_called);
}
