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

