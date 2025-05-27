#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include "iarmtimerstatussubscriber.h"
#include "iarmtimerstatussubscriber.cpp"  // Only include the cpp file if necessary (no .h available)
#include "iarmsubscribe.cpp"              // Ensure this is safe to include for base class logic
#include "pwrMgr.h."

using ::testing::_;
using ::testing::Return;

class MockIARM {
public:
    MOCK_METHOD(IARM_Result_t, IsConnected, (const char*, int*), ());
    MOCK_METHOD(IARM_Result_t, Init, (const char*), ());
    MOCK_METHOD(IARM_Result_t, Connect, (), ());
    MOCK_METHOD(IARM_Result_t, RegisterCall, (const char*, IARM_BusCall_t), ());
    MOCK_METHOD(IARM_Result_t, RegisterEventHandler,
            (const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler), ());


};

static MockIARM* gMockIARM = nullptr;

// C-linkage wrappers to redirect to the mock
extern "C" {
    IARM_Result_t IARM_Bus_IsConnected(const char* name, int* isRegistered) {
        return gMockIARM->IsConnected(name, isRegistered);
    }

    IARM_Result_t IARM_Bus_Init(const char* name) {
        return gMockIARM->Init(name);
    }

    IARM_Result_t IARM_Bus_Connect() {
        return gMockIARM->Connect();
    }

    IARM_Result_t IARM_Bus_RegisterCall(const char* name, IARM_BusCall_t handler) {
        return gMockIARM->RegisterCall(name, handler);
    }

    IARM_Result_t IARM_Bus_RegisterEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
        return gMockIARM->RegisterEventHandler(ownerName, eventId, handler);
    }
}

class IarmTimerStatusSubscriberTest : public ::testing::Test {
protected:
    void SetUp() override {
        gMockIARM = new MockIARM;
    }

    void TearDown() override {
        delete gMockIARM;
        gMockIARM = nullptr;
    }
};

TEST_F(IarmTimerStatusSubscriberTest, Constructor_IarmNotConnected_InitializesAndConnects) {
    EXPECT_CALL(*gMockIARM, IsConnected(_, _))
        .WillOnce([](const char*, int* reg) {
            *reg = 0;
            return IARM_RESULT_INVALID_STATE;
        });

    EXPECT_CALL(*gMockIARM, Init(_)).Times(1);
    EXPECT_CALL(*gMockIARM, Connect()).Times(1);

    IarmTimerStatusSubscriber subscriber("test_subscriber");
}

TEST_F(IarmTimerStatusSubscriberTest, Constructor_IarmAlreadyConnected_DoesNotReconnect) {
    EXPECT_CALL(*gMockIARM, IsConnected(_, _))
        .WillOnce([](const char*, int* reg) {
            *reg = 1;
            return IARM_RESULT_SUCCESS;
        });

    EXPECT_CALL(*gMockIARM, Init(_)).Times(0);
    EXPECT_CALL(*gMockIARM, Connect()).Times(0);

    IarmTimerStatusSubscriber subscriber("test_subscriber");
}

TEST_F(IarmTimerStatusSubscriberTest, Subscribe_ValidEventName_RegistersCallback) {
    IarmTimerStatusSubscriber subscriber("test_subscriber");

    EXPECT_CALL(*gMockIARM, RegisterCall(testing::_, testing::_))
        .WillOnce(testing::Return(IARM_RESULT_SUCCESS));

    EXPECT_CALL(*gMockIARM, RegisterEventHandler(IARM_BUS_RDKPWR_NAME, IARM_BUS_RDKPWR_EVENT_TIMER_STATUS, testing::_))
        .WillOnce(testing::Return(IARM_RESULT_SUCCESS));

    bool result = subscriber.subscribe(TIMER_STATUS_MSG, reinterpret_cast<funcPtr>(0x1234));
    EXPECT_TRUE(result);
}


TEST_F(IarmTimerStatusSubscriberTest, Subscribe_InvalidEventName_ReturnsFalse) {
    IarmTimerStatusSubscriber subscriber("test_subscriber");

    EXPECT_CALL(*gMockIARM, RegisterCall(_, _)).Times(0);  // Should not be called

    bool result = subscriber.subscribe("INVALID_EVENT", reinterpret_cast<funcPtr>(0x1234));
    EXPECT_FALSE(result);
}

