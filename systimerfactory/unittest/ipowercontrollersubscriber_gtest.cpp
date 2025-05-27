#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)


#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include "ipowercontrollersubscriber.h"
#include "ipowercontrollersubscriber.cpp"

// Mocking the external PowerController API functions
class MockPowerController {
public:
    MOCK_METHOD(void, PowerController_Init, (), ());
    MOCK_METHOD(uint32_t, PowerController_Connect, (), ());
    MOCK_METHOD(uint32_t, PowerController_RegisterPowerModeChangedCb, (PowerController_PowerModeChangedCb, void*), ());
    MOCK_METHOD(uint32_t, PowerController_UnRegisterPowerModeChangedCb, (PowerController_PowerModeChangedCb), ());
    MOCK_METHOD(void, PowerController_Term, (), ());
};

static MockPowerController* gMockPowerController = nullptr;

// Wrapper functions to redirect calls to mocks
extern "C" {
    void PowerController_Init() {
        gMockPowerController->PowerController_Init();
    }

    uint32_t  PowerController_Connect() {
        return gMockPowerController->PowerController_Connect();
    }

    uint32_t  PowerController_RegisterPowerModeChangedCb(PowerController_PowerModeChangedCb cb, void* userData) {
        return gMockPowerController->PowerController_RegisterPowerModeChangedCb(cb, userData);
    }

    uint32_t  PowerController_UnRegisterPowerModeChangedCb(PowerController_PowerModeChangedCb cb) {
        return gMockPowerController->PowerController_UnRegisterPowerModeChangedCb(cb);
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

TEST_F(IpowerControllerSubscriberTest, Subscribe_PowerControllerConnectSuccess_RegistersCallback) {
    IpowerControllerSubscriber subscriber("test_subscriber");

    EXPECT_CALL(mockPowerController, PowerController_Init()).Times(1);
    EXPECT_CALL(mockPowerController, PowerController_Connect()).WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));
    EXPECT_CALL(mockPowerController, PowerController_RegisterPowerModeChangedCb(::testing::_, nullptr))
        .WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));

    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);

    EXPECT_TRUE(ret);
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_PowerControllerConnectFailure_StartsThread) {
    IpowerControllerSubscriber subscriber("test_subscriber");

    EXPECT_CALL(mockPowerController, PowerController_Init()).Times(1);
    EXPECT_CALL(mockPowerController, PowerController_Connect())
        .WillOnce(::testing::Return(-1))  // Fail first connect
        .WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE)); // Succeed next time

    // RegisterPowerModeChangedCallback should be called eventually in thread, allow call once
    EXPECT_CALL(mockPowerController, PowerController_RegisterPowerModeChangedCb(::testing::_, nullptr))
        .WillOnce(::testing::Return(POWER_CONTROLLER_ERROR_NONE));

    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);

    EXPECT_TRUE(ret);

    // NOTE: Can't easily verify detached thread internals here, but coverage that thread was started is implicit
}

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

