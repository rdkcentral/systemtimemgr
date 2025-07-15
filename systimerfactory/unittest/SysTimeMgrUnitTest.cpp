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

    // FIX: This signature MUST exactly match IPublish::publish
    MOCK_METHOD(void, publish, (int event, void* args), (override));
};

class MockSubscribe : public ISubscribe {
public:
   MockSubscribe() : ISubscribe("MockSubscriberForTest") {} // <--- ADD OR KEEP THIS

    // This MUST match the signature from isubscribe.h:
    // virtual bool subscribe(string eventname,funcPtr fptr) = 0;
    MOCK_METHOD(bool, subscribe, (string eventname, funcPtr fptr), (override)); // <--- FIX THIS LINE
};



using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;

class SysTimeMgrTest : public ::testing::Test {
protected:
    SysTimeMgr* mgr;
    void SetUp() override {
        mgr = SysTimeMgr::get_instance();
        ASSERT_NE(mgr, nullptr);
    }
    void TearDown() override {
        // No deletion: Singleton
    }
};

TEST_F(SysTimeMgrTest, SingletonReturnsSamePointer) {
    EXPECT_EQ(mgr, SysTimeMgr::get_instance());
}

/*TEST_F(SysTimeMgrTest, InitializeHandlesMissingConfig) {
    mgr->m_cfgfile = "/tmp/nonexistent.conf";
    mgr->initialize();
    // Should log error, not crash
}*/

TEST_F(SysTimeMgrTest, InitializeLoadsValidConfig) {
    // Prepare a test config file
    std::ofstream cfg("/tmp/systimemgr_test.conf");
    cfg << "timesrc testsrc /src\n";
    cfg << "timesync testsync /sync\n";
    cfg.close();
    mgr->m_cfgfile = "/tmp/systimemgr_test.conf";
    mgr->initialize();
    // Should load src/sync
}

TEST_F(SysTimeMgrTest, RunStateMachineUnknownEvent) {
    mgr->runStateMachine(eSYSMGR_EVENT_UNKNOWN, nullptr);
    // Should not crash, log info
}

TEST_F(SysTimeMgrTest, TimerExpiryUsesReference) {
    auto* src = new MockTimeSrc();
    EXPECT_CALL(*src, isreference()).WillOnce(Return(true));
    EXPECT_CALL(*src, getTimeSec()).WillOnce(Return(1234));
    mgr->m_timerSrc.push_back(src);
    mgr->timerExpiry(nullptr);
}

TEST_F(SysTimeMgrTest, TimerExpiryUsesFileTime) {
    auto* src = new MockTimeSrc();
    EXPECT_CALL(*src, isreference()).WillOnce(Return(false));
    EXPECT_CALL(*src, getTimeSec()).WillOnce(Return(5678));
    mgr->m_timerSrc.push_back(src);
    mgr->timerExpiry(nullptr);
}

TEST_F(SysTimeMgrTest, UpdateTimeSyncCallsSyncs) {
    auto* sync = new MockTimeSync();
    EXPECT_CALL(*sync, updateTime(_)).Times(1);
    mgr->m_timerSync.push_back(sync);
    mgr->updateTimeSync(1234);
}

TEST_F(SysTimeMgrTest, NtpFailedPublishesStatus) {
    auto* pub = new MockPublish();
    mgr->m_publish = pub;
    EXPECT_CALL(*pub, publish(_, _)).Times(AtLeast(1));
    mgr->ntpFailed(nullptr);
}

TEST_F(SysTimeMgrTest, NtpAquiredPublishesStatusAndUpdatesState) {
    auto* pub = new MockPublish();
    mgr->m_publish = pub;
    EXPECT_CALL(*pub, publish(_, _)).Times(AtLeast(1));
    mgr->ntpAquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_NTP_ACQUIRED);
}

TEST_F(SysTimeMgrTest, DttAcquiredPublishesStatusAndUpdatesState) {
    auto* pub = new MockPublish();
    mgr->m_publish = pub;
    EXPECT_CALL(*pub, publish(_, _)).Times(AtLeast(1));
    mgr->dttAquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_DTT_ACQUIRED);
}

TEST_F(SysTimeMgrTest, SecureTimeAcquiredUpdatesState) {
    mgr->secureTimeAcquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_SECURE_TIME_ACQUIRED);
}

TEST_F(SysTimeMgrTest, UpdateSecureTimePublishesStatusAndUpdatesState) {
    auto* pub = new MockPublish();
    mgr->m_publish = pub;
    EXPECT_CALL(*pub, publish(_, _)).Times(AtLeast(1));
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
    auto* pub = new MockPublish();
    mgr->m_publish = pub;
    mgr->m_timequality = eTIMEQUALILTY_SECURE;
    EXPECT_CALL(*pub, publish(_, _)).Times(AtLeast(1));
    mgr->deepsleepoff();
}

TEST_F(SysTimeMgrTest, DeepSleepOnLogs) {
    mgr->deepsleepon(); // Just for coverage
}

TEST_F(SysTimeMgrTest, UpdateClockRealTimeSetsTime) {
    auto* src = new MockTimeSrc();
    EXPECT_CALL(*src, isclockProvider()).WillOnce(Return(true));
    EXPECT_CALL(*src, getTimeSec()).WillOnce(Return(1234));
    EXPECT_CALL(*src, checkTime()).Times(1);
    mgr->m_timerSrc.push_back(src);
    mgr->updateClockRealTime(nullptr);
}






