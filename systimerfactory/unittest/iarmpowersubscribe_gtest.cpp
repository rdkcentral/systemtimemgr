#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s] " format "\n", #level, #module, ##__VA_ARGS__)

#include <string>
#include <cstring>
#include "iarmsubscribe.h"
#include "iarmpowersubscriber.h"
#include "iarmpowersubscriber.cpp"  // Use only if no header is available
#include "iarmsubscribe.cpp" 
using ::testing::_;
using ::testing::Return;
using std::string;

// --- Mocks for IARM ---
class MockIARM {
public:
    MOCK_METHOD(IARM_Result_t, IsConnected, (const char*, int*), ());
    MOCK_METHOD(IARM_Result_t, Init, (const char*), ());
    MOCK_METHOD(IARM_Result_t, Connect, (), ());
    MOCK_METHOD(IARM_Result_t, RegisterEventHandler, (const char*,  IARM_EventId_t, IARM_EventHandler_t), ());
};

static MockIARM* gMockIARM = nullptr;

extern "C" {
    IARM_Result_t IARM_Bus_IsConnected(const char* name, int* reg) {
        return gMockIARM->IsConnected(name, reg);
    }
    IARM_Result_t IARM_Bus_Init(const char* name) {
        return gMockIARM->Init(name);
    }
    IARM_Result_t IARM_Bus_Connect() {
        return gMockIARM->Connect();
    }
    IARM_Result_t IARM_Bus_RegisterEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
        return gMockIARM->RegisterEventHandler(ownerName, eventId, handler);
    }
}

// --- Globals for handler verification ---
static std::string gReceivedStatus;

// --- Custom handler to capture event data ---
// Changed void->int return to match funcPtr signature
int TestPowerHandler(void* data) {
    std::string* status = static_cast<std::string*>(data);
    gReceivedStatus = *status;
    return 0;
}

// --- Fixture ---
class IarmPowerSubscriberTest : public ::testing::Test {
protected:
    void SetUp() override {
        gMockIARM = new MockIARM;
        gReceivedStatus.clear();
    }

    void TearDown() override {
        delete gMockIARM;
        gMockIARM = nullptr;
    }
};

// --- Tests ---

TEST_F(IarmPowerSubscriberTest, Constructor_NotConnected_CallsInitAndConnect) {
    EXPECT_CALL(*gMockIARM, IsConnected(_, _)).WillOnce([](const char*, int* reg) {
        *reg = 0;
        return IARM_RESULT_INVALID_STATE;
    });
    EXPECT_CALL(*gMockIARM, Init(_)).Times(1);
    EXPECT_CALL(*gMockIARM, Connect()).Times(1);
    IarmPowerSubscriber subscriber("TestSub");
}

TEST_F(IarmPowerSubscriberTest, Subscribe_ValidPowerEvent_Success) {
    IarmPowerSubscriber subscriber("TestSub");
    EXPECT_CALL(*gMockIARM, IsConnected(_, _))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*gMockIARM, RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, _))
        .WillOnce(Return(IARM_RESULT_SUCCESS));
    bool result = subscriber.subscribe(POWER_CHANGE_MSG, TestPowerHandler);
    EXPECT_TRUE(result);
}

TEST_F(IarmPowerSubscriberTest, Subscribe_InvalidEvent_ReturnsFalse) {
    IarmPowerSubscriber subscriber("TestSub");
    bool result = subscriber.subscribe("UNKNOWN_EVENT", TestPowerHandler);
    EXPECT_FALSE(result);
}

TEST_F(IarmPowerSubscriberTest, PowerEventHandler_DeepSleepOn_TriggersHandler) {
    IarmPowerSubscriber subscriber("TestSub");
    subscriber.subscribe(POWER_CHANGE_MSG, TestPowerHandler);

    IARM_Bus_PWRMgr_EventData_t evt {};
    evt.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
    evt.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_ON;

    IarmPowerSubscriber::powereventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &evt, sizeof(evt));
    EXPECT_EQ(gReceivedStatus, "DEEP_SLEEP_ON");
}

TEST_F(IarmPowerSubscriberTest, PowerEventHandler_DeepSleepOff_TriggersHandler) {
    IarmPowerSubscriber subscriber("TestSub");
    subscriber.subscribe(POWER_CHANGE_MSG, TestPowerHandler);

    IARM_Bus_PWRMgr_EventData_t evt {};
    evt.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
    evt.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_ON;

    IarmPowerSubscriber::powereventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, &evt, sizeof(evt));
    EXPECT_EQ(gReceivedStatus, "DEEP_SLEEP_OFF");
}

TEST_F(IarmPowerSubscriberTest, PowerEventHandler_UnrelatedEventId_DoesNothing) {
    IarmPowerSubscriber subscriber("TestSub");
    subscriber.subscribe(POWER_CHANGE_MSG, TestPowerHandler);

    gReceivedStatus.clear();
    IarmPowerSubscriber::powereventHandler(IARM_BUS_PWRMGR_NAME, 999, nullptr, 0);
    EXPECT_TRUE(gReceivedStatus.empty());
}
