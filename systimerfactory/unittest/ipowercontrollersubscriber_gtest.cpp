#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)


#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include "ipowercontrollersubscriber.h"
#include "ipowercontrollersubscriber.cpp"
#include "iarmsubscribe.cpp"

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
class IarmSubscriberTestHelper : public IarmSubscriber {
public:
    static void setInstance(IarmSubscriber* inst) { IarmSubscriber::pInstance = inst; }
};


class IpowerControllerSubscriberTest : public ::testing::Test {
protected:
    void SetUp() override {
        gMockPowerController = &mockPowerController;
          IarmSubscriberTestHelper::setInstance(&subscriber);
    }

    void TearDown() override {
        gMockPowerController = nullptr;
    }

    MockPowerController mockPowerController;
    IpowerControllerSubscriber subscriber{"test_subscriber"};
};





TEST_F(IpowerControllerSubscriberTest, Subscribe_InvalidEventName_ReturnsFalse) {
    IpowerControllerSubscriber subscriber("test_subscriber");

    bool ret = subscriber.subscribe("INVALID_EVENT", nullptr);

    EXPECT_FALSE(ret);
}


static bool handlerCalled = false;
static int testHandler(void* status) {
    handlerCalled = true;
    std::string* str = static_cast<std::string*>(status);
    EXPECT_EQ(*str, "DEEP_SLEEP_ON"); // or whatever value you expect
    return 0;
}
TEST_F(IpowerControllerSubscriberTest, Destructor_CallsPowerControllerTerm) {
    EXPECT_CALL(mockPowerController, PowerController_Term()).Times(1);

    // Ensure singleton is set
    IarmSubscriberTestHelper::setInstance(&subscriber);

    // Optionally, start the event thread so join is legal
    subscriber.sysTimeMgrInitPwrEvt();

    // Optionally, set power handler for thread
    subscriber.m_powerHandler = testHandler;

    // Optionally, send a dummy event so thread wakes up
    subscriber.m_pwrEvtCondVar.notify_one();

    // Destroy subscriber (scope exit)
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

TEST_F(IpowerControllerSubscriberTest, Subscribe_ConnectSuccess_CallbackSuccess) {
    EXPECT_CALL(mockPowerController, PowerController_Init()).Times(1);
    EXPECT_CALL(mockPowerController, PowerController_Connect()).WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));
    EXPECT_CALL(mockPowerController, PowerController_RegisterPowerModeChangedCallback(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));
    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, testHandler);
    EXPECT_TRUE(ret);
}

