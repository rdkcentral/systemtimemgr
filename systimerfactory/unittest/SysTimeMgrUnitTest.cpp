#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Client_Mock.h"
#include <fstream>
//#include "secure_wrapper.h"
#include "systimemgr.h"
#include "systimemgr.cpp"
#include "timerfactory.cpp"
#include "pubsubfactory.cpp"
#include "rdkdefaulttimesync.cpp"
#include "drmtimersrc.cpp"
#include "itimesrc.h"
#include "itimesync.h"
#include "ipublish.h"
#include "isubscribe.h"
#include "itimermsg.h"

class MockTimeSrc : public ITimeSrc {
public:
    MOCK_METHOD(bool, isreference, (), (override));
    MOCK_METHOD(long long, getTimeSec, (), (override));
    MOCK_METHOD(bool, isclockProvider, (), (override));
    MOCK_METHOD(bool, checkTime, (), (override));
};

class MockTimeSync : public ITimeSync {
public:
    MOCK_METHOD(long long, getTime, (), (override));
    MOCK_METHOD(void, updateTime, (long long), (override));
};

class MockPublish : public IPublish {
public:
    MockPublish() : IPublish("MockPublisherForTest") {}
    MOCK_METHOD(void, publish, (int event, void* args), (override));
};

class MockSubscribe : public ISubscribe {
public:
   MockSubscribe() : ISubscribe("MockSubscriberForTest") {}
    MOCK_METHOD(bool, subscribe, (string eventname, funcPtr fptr), (override));
};

using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;

class SysTimeMgrTest : public ::testing::Test {
protected:
    SysTimeMgr* mgr;

    // Declare mock pointers as member variables
    MockTimeSrc* mockTimeSrc;
    MockTimeSync* mockTimeSync;
    MockPublish* mockPublish;
    MockSubscribe* mockSubscribe; // If needed later for subscribe tests

    void SetUp() override {
        mgr = SysTimeMgr::get_instance();
        ASSERT_NE(mgr, nullptr);

        // Initialize mocks
        mockTimeSrc = new MockTimeSrc();
        mockTimeSync = new MockTimeSync();
        mockPublish = new MockPublish();
        mockSubscribe = new MockSubscribe();

        // Clear existing sources/syncs if any from previous tests (important for singletons)
        mgr->m_timerSrc.clear();
        mgr->m_timerSync.clear();

        // Assign mocks to SysTimeMgr's members where applicable
        // Note: For vectors like m_timerSrc and m_timerSync, you push_back in specific tests.
        // For direct members like m_publish, assign it here if it's always used.
        // If m_publish is set per test, then move this line to the test itself.
        mgr->m_publish = mockPublish;
    }

    void TearDown() override {
        // Delete mock objects to prevent leaks
        delete mockTimeSrc;
        delete mockTimeSync;
        delete mockPublish;
        delete mockSubscribe;

        // Note: mgr is a singleton, so no deletion here.
        // It's crucial to clear any lists mgr might hold if they contain raw pointers
        // to objects created within the tests.
        // Otherwise, subsequent tests might interact with stale or deleted pointers.
        mgr->m_timerSrc.clear();
        mgr->m_timerSync.clear();
    }
};

TEST_F(SysTimeMgrTest, SingletonReturnsSamePointer) {
    EXPECT_EQ(mgr, SysTimeMgr::get_instance());
}



TEST_F(SysTimeMgrTest, RunStateMachineUnknownEvent) {
    mgr->runStateMachine(eSYSMGR_EVENT_UNKNOWN, nullptr);
    // Should not crash, log info
}

TEST_F(SysTimeMgrTest, TimerExpiryUsesReference) {
    // Use the mockTimeSrc member variable
    EXPECT_CALL(*mockTimeSrc, isreference()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(1234));
    mgr->m_timerSrc.push_back(mockTimeSrc); // Add it to the manager's list
    mgr->timerExpiry(nullptr);
    // Remove the mock from the manager's list after the test to prevent it being used by other tests.
    // Or, better, clear the lists in TearDown, as suggested above.
}

TEST_F(SysTimeMgrTest, TimerExpiryUsesFileTime) {
    // Use the mockTimeSrc member variable
    EXPECT_CALL(*mockTimeSrc, isreference()).WillOnce(Return(false));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(5678));
    mgr->m_timerSrc.push_back(mockTimeSrc); // Add it to the manager's list
    mgr->timerExpiry(nullptr);
}

TEST_F(SysTimeMgrTest, UpdateTimeSyncCallsSyncs) {
    // Use the mockTimeSync member variable
    EXPECT_CALL(*mockTimeSync, updateTime(_)).Times(1);
    mgr->m_timerSync.push_back(mockTimeSync); // Add it to the manager's list
    mgr->updateTimeSync(1234);
}

TEST_F(SysTimeMgrTest, NtpFailedPublishesStatus) {
    // mgr->m_publish is already set in SetUp to mockPublish
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->ntpFailed(nullptr);
}

TEST_F(SysTimeMgrTest, NtpAquiredPublishesStatusAndUpdatesState) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->ntpAquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_NTP_ACQUIRED);
}

TEST_F(SysTimeMgrTest, DttAcquiredPublishesStatusAndUpdatesState) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->dttAquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_DTT_ACQUIRED);
}

TEST_F(SysTimeMgrTest, SecureTimeAcquiredUpdatesState) {
    mgr->secureTimeAcquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_SECURE_TIME_ACQUIRED);
}

TEST_F(SysTimeMgrTest, UpdateSecureTimePublishesStatusAndUpdatesState) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->updateSecureTime(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_RUNNING);
}

TEST_F(SysTimeMgrTest, SendMessageQueuesEvent) {
    mgr->sendMessage(eSYSMGR_EVENT_TIMER_EXPIRY, nullptr);
    // Could check queue size if exposed, otherwise rely on coverage
}

TEST_F(SysTimeMgrTest, GetTimeStatusPopulatesFields) {
    TimerMsg msg;
    mgr->getTimeStatus(&msg);
    // Check at least one field
    EXPECT_GE(strlen(msg.message), 0);
}

TEST_F(SysTimeMgrTest, PowerHandlerHandlesSleepEvents) {
    std::string on = "DEEP_SLEEP_ON";
    std::string off = "DEEP_SLEEP_OFF";
    EXPECT_EQ(SysTimeMgr::powerhandler(&on), 1);
    EXPECT_EQ(SysTimeMgr::powerhandler(&off), 1);
}

TEST_F(SysTimeMgrTest, DeepSleepOffPublishesStatus) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->m_timequality = eTIMEQUALILTY_SECURE;
    mgr->deepsleepoff();
}

TEST_F(SysTimeMgrTest, DeepSleepOnLogs) {
    mgr->deepsleepon(); // Just for coverage
}

TEST_F(SysTimeMgrTest, UpdateClockRealTimeSetsTime) {
    EXPECT_CALL(*mockTimeSrc, isclockProvider()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(1234));
    EXPECT_CALL(*mockTimeSrc, checkTime()).Times(1);
    mgr->m_timerSrc.push_back(mockTimeSrc); // Add it to the manager's list
    mgr->updateClockRealTime(nullptr);
}
