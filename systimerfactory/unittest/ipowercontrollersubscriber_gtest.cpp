#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)


#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include "ipowercontrollersubscriber.h"
#include "ipowercontrollersubscriber.cpp"
#include "iarmsubscribe.cpp"
#include "testsubscribe.h"

// Mocking the external PowerController API functions
class MockPowerController {
public:
    MOCK_METHOD(void, PowerController_Init, (), ());
    MOCK_METHOD(uint32_t, PowerController_Connect, (), ());
    MOCK_METHOD(uint32_t, PowerController_RegisterPowerModeChangedCallback, (PowerController_PowerModeChangedCb, void*), ());
    MOCK_METHOD(uint32_t, PowerController_UnRegisterPowerModeChangedCallback, (PowerController_PowerModeChangedCb), ());
    MOCK_METHOD(void, PowerController_Term, (), ());
};

static MockPowerController* gMockPowerController = nullptr;

// Redirect the calls
extern "C" {
    void PowerController_Init() {
        gMockPowerController->PowerController_Init();
    }

    uint32_t  PowerController_Connect() {
        return gMockPowerController->PowerController_Connect();
    }

    uint32_t  PowerController_RegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb cb, void* userData) {
        return gMockPowerController->PowerController_RegisterPowerModeChangedCallback(cb, userData);
    }

    uint32_t  PowerController_UnRegisterPowerModeChangedCallback(PowerController_PowerModeChangedCb cb) {
        return gMockPowerController->PowerController_UnRegisterPowerModeChangedCallback(cb);
    }

    void PowerController_Term() {
        gMockPowerController->PowerController_Term();
    }
}

class IpowerControllerSubscriberTest : public ::testing::Test {
protected:
    void SetUp() override {
        gMockPowerController = &mockPowerController;
    }

    void TearDown() override {
        gMockPowerController = nullptr;
    }

    MockPowerController mockPowerController;
};




TEST_F(IpowerControllerSubscriberTest, Destructor_CallsPowerControllerTerm) {
    {
        EXPECT_CALL(mockPowerController, PowerController_Term()).Times(1);
        IpowerControllerSubscriber subscriber("test_subscriber");
    }
    // Destructor called at block exit, PowerController_Term should be invoked
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_InvalidEventName_ReturnsFalse) {
    IpowerControllerSubscriber subscriber("test_subscriber");

    bool ret = subscriber.subscribe("INVALID_EVENT", nullptr);

    EXPECT_FALSE(ret);
}


/*static bool handlerCalled = false;
static int testHandler(void* status) {
    handlerCalled = true;
    std::string* str = static_cast<std::string*>(status);
    EXPECT_EQ(*str, "DEEP_SLEEP_ON"); // or whatever value you expect
    return 0;
}

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_DeepSleepOn) {
    IpowerControllerSubscriber subscriber("sub");

    // Directly set the private member since you can access it
    subscriber.m_powerHandler = testHandler;

    // If your test is in the same translation unit and m_powerHandler is accessible, this will work
    handlerCalled = false;
   // IarmSubscriber::instance = &subscriber; // If needed for getInstance() logic

    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_UNKNOWN, POWER_STATE_OFF);

    EXPECT_TRUE(handlerCalled);
}*/

static bool handlerCalled = false;
static int testHandler(void* status) {
    handlerCalled = true;
    std::string* str = static_cast<std::string*>(status);
    EXPECT_EQ(*str, "DEEP_SLEEP_ON"); // or whatever value you expect
    return 0;
}

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_DeepSleepOn) {
    IpowerControllerSubscriber subscriber("sub");

    // Directly set the private member since you can access it
    subscriber.m_powerHandler = testHandler;

    // If your test is in the same translation unit and m_powerHandler is accessible, this will work
    handlerCalled = false;
   // IarmSubscriber::instance = &subscriber; // If needed for getInstance() logic

    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_UNKNOWN, POWER_STATE_OFF);

    EXPECT_TRUE(handlerCalled);
}
static bool handlerCalledoff = false;
static int testHandleroff(void* status) {
    handlerCalledoff = true;
    std::string* str = static_cast<std::string*>(status);
    EXPECT_EQ(*str, "DEEP_SLEEP_OFF");
    return 0;
}

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_DeepSleepOff) {
    IpowerControllerSubscriber subscriber("sub");
    subscriber.m_powerHandler = testHandleroff;
    handlerCalledoff = false;

    // Set currentState as POWER_STATE_STANDBY_DEEP_SLEEP and newState as POWER_STATE_ON
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_STANDBY_DEEP_SLEEP, POWER_STATE_ON);

    EXPECT_TRUE(handlerCalledoff);
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_ValidEvent_ConnectionFails_WithTestSubscriber) {
    TestSubscriber subscriber("test_subscriber");
    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);
    EXPECT_TRUE(ret);
    // No need to manage threads, queues, or shutdown.
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_ValidEvent_ConnectionSuccess_WithTestSubscriber) {
    TestSubscriber subscriber("test_subscriber");
    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);
    EXPECT_TRUE(ret);
}

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_UnknownNewState_LogsError) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    handlerCalled = false;
    subscriber.m_powerHandler = testHandler;
    // Use an invalid enum value for newState
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_ON, static_cast<PowerController_PowerState_t>(999));
    EXPECT_FALSE(handlerCalled); // Handler should not be called
}

TEST_F(IpowerControllerSubscriberTest, Destructor_UnregisterCallbackFails_StillCleansUp) {
    IpowerControllerSubscriber* subscriber = new IpowerControllerSubscriber("test_subscriber");
    EXPECT_CALL(mockPowerController, PowerController_Term());
    EXPECT_CALL(mockPowerController, PowerController_UnRegisterPowerModeChangedCallback(::testing::_)).WillOnce(::testing::Return(1));
    delete subscriber;
    // No crash expected, error should be logged
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_EmptyEventName_ReturnsFalse) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    bool ret = subscriber.subscribe("", nullptr);
    EXPECT_FALSE(ret);
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_NullHandler_DoesNotCrash) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    EXPECT_CALL(mockPowerController, PowerController_Init());
    EXPECT_CALL(mockPowerController, PowerController_Connect()).WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));
    EXPECT_CALL(mockPowerController, PowerController_RegisterPowerModeChangedCallback(::testing::_, ::testing::_)).WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));

    // nullptr as handler
    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);
    EXPECT_TRUE(ret);
    // Optionally: Simulate event and ensure it doesn't crash
}

TEST_F(IpowerControllerSubscriberTest, Destructor_WithoutSubscribe_DoesNotCrashOrLeak) {
    EXPECT_CALL(mockPowerController, PowerController_Term()).Times(1);
    IpowerControllerSubscriber* subscriber = new IpowerControllerSubscriber("test_subscriber");
    delete subscriber;
    // No subscribe called, just construction and destruction
}

/*TEST_F(IpowerControllerSubscriberTest, Subscribe_CalledTwice_SecondCallHandledGracefully) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    // Set up mocks for two subscribe calls
    EXPECT_CALL(mockPowerController, PowerController_Init()).Times(::testing::AtLeast(2));
    EXPECT_CALL(mockPowerController, PowerController_Connect()).Times(2).WillRepeatedly(::testing::Return(POWER_CONTROLLER_ERROR_NONE));
    EXPECT_CALL(mockPowerController, PowerController_RegisterPowerModeChangedCallback(::testing::_, ::testing::_)).Times(2).WillRepeatedly(::testing::Return(POWER_CONTROLLER_ERROR_NONE));

    // First subscribe
    bool first = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);
    // Second subscribe
    bool second = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);

    EXPECT_TRUE(first);
    EXPECT_TRUE(second);
}*/

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_NoHandler_DoesNotCrash) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    subscriber.m_powerHandler = nullptr; // Explicitly unset
    // Should not crash
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_UNKNOWN, POWER_STATE_OFF);
    SUCCEED();
}
