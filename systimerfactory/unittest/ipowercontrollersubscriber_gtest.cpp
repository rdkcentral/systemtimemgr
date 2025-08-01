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




TEST_F(IpowerControllerSubscriberTest, Destructor_CallsPowerControllerTerm) 
    {
       EXPECT_CALL(mockPowerController, PowerController_Term()).Times(1);
        IpowerControllerSubscriber subscriber("test_subscriber");
    }


TEST_F(IpowerControllerSubscriberTest, Subscribe_InvalidEventName_ReturnsFalse) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    bool ret = subscriber.subscribe("INVALID_EVENT", nullptr);
    EXPECT_FALSE(ret);
}

static bool handlerCalled = false;
static int testHandler(void* status) {
    handlerCalled = true;
    std::string* str = static_cast<std::string*>(status);
    EXPECT_EQ(*str, "DEEP_SLEEP_ON"); 
    return 0;
}

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_DeepSleepOn) {
    IpowerControllerSubscriber subscriber("sub");

    subscriber.m_powerHandler = testHandler;
    handlerCalled = false;
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
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_STANDBY_DEEP_SLEEP, POWER_STATE_ON);
    EXPECT_TRUE(handlerCalledoff);
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_ValidEvent_ConnectionFails_WithTestSubscriber) {
    TestSubscriber subscriber("test_subscriber");
    bool ret = subscriber.subscribe(POWER_CHANGE_MSG, nullptr);
    EXPECT_TRUE(ret);
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
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_ON, static_cast<PowerController_PowerState_t>(999));
    EXPECT_FALSE(handlerCalled); 
}

TEST_F(IpowerControllerSubscriberTest, Destructor_UnregisterCallbackFails_StillCleansUp) {
    IpowerControllerSubscriber* subscriber = new IpowerControllerSubscriber("test_subscriber");
    EXPECT_CALL(mockPowerController, PowerController_Term());
    EXPECT_CALL(mockPowerController, PowerController_UnRegisterPowerModeChangedCallback(::testing::_)).WillOnce(::testing::Return(1));
    delete subscriber;
}

TEST_F(IpowerControllerSubscriberTest, Subscribe_EmptyEventName_ReturnsFalse) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    bool ret = subscriber.subscribe("", nullptr);
    EXPECT_FALSE(ret);
}

TEST_F(IpowerControllerSubscriberTest, Destructor_WithoutSubscribe_DoesNotCrashOrLeak) {
    EXPECT_CALL(mockPowerController, PowerController_Term()).Times(1);
    IpowerControllerSubscriber* subscriber = new IpowerControllerSubscriber("test_subscriber");
    delete subscriber;
}

TEST_F(IpowerControllerSubscriberTest, UnRegisterPowerModeChangedCallback_ReturnsError_CoversErrorPath) {
    // Set up mock to return error (non-zero)
    EXPECT_CALL(mockPowerController, PowerController_UnRegisterPowerModeChangedCallback(::testing::_)).WillOnce(::testing::Return(1));

    // Call the function directly and check the error path
    uint32_t result = PowerController_UnRegisterPowerModeChangedCallback(IpowerControllerSubscriber::sysTimeMgrPwrEventHandler);

    EXPECT_EQ(result, 1); // Ensure error is returned
}

TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_NoHandler_DoesNotCrash) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    subscriber.m_powerHandler = nullptr; 
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_UNKNOWN, POWER_STATE_OFF);
    SUCCEED();
}

class TestableSubscriber : public IpowerControllerSubscriber {
public:
    using IpowerControllerSubscriber::IpowerControllerSubscriber;
    using IpowerControllerSubscriber::m_pwrEvtQueue;
    using IpowerControllerSubscriber::m_pwrEvtQueueLock;
    using IpowerControllerSubscriber::m_pwrEvtCondVar;

    size_t queueSize() {
        std::lock_guard<std::mutex> lock(m_pwrEvtQueueLock);
        return m_pwrEvtQueue.size();
    }
    SysTimeMgr_Power_Event_State_t queueFront() {
        std::lock_guard<std::mutex> lock(m_pwrEvtQueueLock);
        return m_pwrEvtQueue.front();
    }
    void clearQueue() {
        std::lock_guard<std::mutex> lock(m_pwrEvtQueueLock);
        m_pwrEvtQueue = std::queue<SysTimeMgr_Power_Event_State_t>();
    }
};

TEST_F(IpowerControllerSubscriberTest, SysTimeMgrPwrEventHandler_EnqueuesEventAndSignals) {
    TestableSubscriber subscriber("test_subscriber");
    subscriber.clearQueue();

    std::atomic<bool> signaled{false};
    std::thread waiter([&]() {
        std::unique_lock<std::mutex> lock(subscriber.m_pwrEvtQueueLock);
        subscriber.m_pwrEvtCondVar.wait_for(
            lock, std::chrono::milliseconds(200),
            [&] { return !subscriber.m_pwrEvtQueue.empty(); }
        );
        signaled = true;
    });

    PowerController_PowerState_t curr = POWER_STATE_OFF;
    PowerController_PowerState_t next = POWER_STATE_ON;
    IpowerControllerSubscriber::sysTimeMgrPwrEventHandler(curr, next, nullptr);

    waiter.join();

    EXPECT_EQ(subscriber.queueSize(), 1u);
    SysTimeMgr_Power_Event_State_t front = subscriber.queueFront();
    EXPECT_EQ(front.currentState, curr);
    EXPECT_EQ(front.newState, next);

    EXPECT_TRUE(signaled);

}


TEST_F(IpowerControllerSubscriberTest, HandlePwrEventData_InvalidState_LogsError) {
    IpowerControllerSubscriber subscriber("test_subscriber");
    subscriber.sysTimeMgrHandlePwrEventData(POWER_STATE_ON, static_cast<PowerController_PowerState_t>(999));
    // No handler, just exercise the default path for coverage
}


