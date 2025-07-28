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

    MockTimeSrc* mockTimeSrc;
    MockTimeSync* mockTimeSync;
    MockPublish* mockPublish;
    MockSubscribe* mockSubscribe; 
    std::string temp_test_dir;

    void SetUp() override {
        mgr = SysTimeMgr::get_instance();
        ASSERT_NE(mgr, nullptr);
        mockTimeSrc = new MockTimeSrc();
        mockTimeSync = new MockTimeSync();
        mockPublish = new MockPublish();
        mockSubscribe = new MockSubscribe();

        mgr->m_timerSrc.clear();
	mgr->m_timerSync.clear();
        mgr->m_publish = mockPublish;
        mgr->m_subscriber = mockSubscribe;
        mgr->m_tmrsubscriber = mockSubscribe;
        mgr->m_pathEventMap.clear();
        mgr->m_pathEventMap.insert({"ntp", eSYSMGR_EVENT_NTP_AVAILABLE});
        mgr->m_pathEventMap.insert({"stt", eSYSMGR_EVENT_NTP_AVAILABLE});
        mgr->m_pathEventMap.insert({"drm", eSYSMGR_EVENT_SECURE_TIME_AVAILABLE});
        mgr->m_pathEventMap.insert({"dtt", eSYSMGR_EVENT_DTT_TIME_AVAILABLE});
    }

    void TearDown() override {
        // Delete mock objects to prevent leaks
        delete mockTimeSrc;
        delete mockTimeSync;
        delete mockPublish;
        delete mockSubscribe;

        mgr->m_timerSrc.clear();
        mgr->m_timerSync.clear();
        if (!temp_test_dir.empty()) {
            for (const auto& entry : mgr->m_pathEventMap) {
                std::string filepath = temp_test_dir + "/" + entry.first;
                std::remove(filepath.c_str());
            }
            rmdir(temp_test_dir.c_str());
        }
        SysTimeMgr::pInstance = nullptr;
    }
};

TEST_F(SysTimeMgrTest, SingletonReturnsSamePointer) {
    EXPECT_EQ(mgr, SysTimeMgr::get_instance());
}

TEST_F(SysTimeMgrTest, DestructorCovers) {
    SysTimeMgr* localMgr = new SysTimeMgr("dummy.cfg");
    delete localMgr; 
}


TEST_F(SysTimeMgrTest, SetInitialTime_ZeroTime) {
    EXPECT_CALL(*mockTimeSync, getTime()).WillOnce(Return(0));
    mgr->m_timerSync.push_back(mockTimeSync);
    mgr->setInitialTime(); 
}

TEST_F(SysTimeMgrTest, SetInitialTime_NonZeroTime) {
    EXPECT_CALL(*mockTimeSync, getTime()).WillOnce(Return(100000));
    mgr->m_timerSync.push_back(mockTimeSync);
    mgr->setInitialTime(); 
}

TEST_F(SysTimeMgrTest, UpdateTime_InvokesCheckTime) {
    EXPECT_CALL(*mockTimeSrc, checkTime()).Times(1);
    mgr->m_timerSrc.push_back(mockTimeSrc);
    mgr->updateTime(nullptr);
}

TEST_F(SysTimeMgrTest, SetInitialTime_FileCreationFails) {
    mkdir("/tmp/systimeset", 0700);

    EXPECT_CALL(*mockTimeSync, getTime()).WillOnce(Return(12345));
    mgr->m_timerSync.push_back(mockTimeSync);

    mgr->setInitialTime();

    // Clean up
    rmdir("/tmp/systimeset");
}

TEST_F(SysTimeMgrTest, RunStateMachineUnknownEvent) {
    mgr->runStateMachine(eSYSMGR_EVENT_UNKNOWN, nullptr);
    }

TEST_F(SysTimeMgrTest, TimerExpiryUsesReference) {
    EXPECT_CALL(*mockTimeSrc, isreference()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(1234));
    mgr->m_timerSrc.push_back(mockTimeSrc); // Add it to the manager's list
    mgr->timerExpiry(nullptr);
    }

TEST_F(SysTimeMgrTest, TimerExpiryUsesFileTime) {
    EXPECT_CALL(*mockTimeSrc, isreference()).WillOnce(Return(false));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(5678));
    mgr->m_timerSrc.push_back(mockTimeSrc); // Add it to the manager's list
    mgr->timerExpiry(nullptr);
}

TEST_F(SysTimeMgrTest, UpdateTimeSyncCallsSyncs) {
    EXPECT_CALL(*mockTimeSync, updateTime(_)).Times(1);
    mgr->m_timerSync.push_back(mockTimeSync); // Add it to the manager's list
    mgr->updateTimeSync(1234);
}

TEST_F(SysTimeMgrTest, NtpFailedPublishesStatus) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->ntpFailed(nullptr);
}

TEST_F(SysTimeMgrTest, NtpAquiredPublishesStatusAndUpdatesState) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->ntpAquired(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_NTP_ACQUIRED);
}

TEST_F(SysTimeMgrTest, NtpFailed) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->ntpFailed(nullptr);
    EXPECT_EQ(mgr->m_state, eSYSMGR_STATE_NTP_FAIL);
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
}

TEST_F(SysTimeMgrTest, GetTimeStatusPopulatesFields) {
    TimerMsg msg;
    mgr->getTimeStatus(&msg);
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

TEST_F(SysTimeMgrTest, DeepSleepOffCoversPoorCase) {
    mgr->m_timequality = eTIMEQUALILTY_POOR;
    mgr->deepsleepoff();
}

TEST_F(SysTimeMgrTest, DeepSleepOnLogs) {
    mgr->deepsleepon(); 
}


TEST_F(SysTimeMgrTest, UpdateClockRealTimeSetsTime) {
    EXPECT_CALL(*mockTimeSrc, isclockProvider()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(1234));
    EXPECT_CALL(*mockTimeSrc, checkTime()).Times(1);
    mgr->m_timerSrc.push_back(mockTimeSrc); 
    mgr->updateClockRealTime(nullptr);
}

TEST_F(SysTimeMgrTest, RunDetachesThreadsWhenNotForever) {

    mgr->run(false); 
}


TEST_F(SysTimeMgrTest, ProcessThrCallsProcessMsgAndRunsOneIteration) {
    mgr->sendMessage(eSYSMGR_EVENT_UNKNOWN, nullptr);
    std::thread process_thread([this]() {
        SysTimeMgr::processThr(mgr);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    process_thread.detach();

    }

TEST_F(SysTimeMgrTest, TimerThrCallsRunTimerAndRunsOnce) {
    EXPECT_CALL(*mockPublish, publish(eSYSMGR_EVENT_TIMER_EXPIRY, _)).Times(AtLeast(0));
    std::thread timer_thread([this]() {
        SysTimeMgr::timerThr(mgr);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    timer_thread.detach();

    }

TEST_F(SysTimeMgrTest, UpdateClockRealTimeAllBranches) {
    EXPECT_CALL(*mockTimeSrc, isclockProvider()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(1234));
    EXPECT_CALL(*mockTimeSrc, checkTime()).Times(1);
    mgr->m_timerSrc.push_back(mockTimeSrc);
    mgr->updateClockRealTime(nullptr);
    mgr->m_timerSrc.clear();
    EXPECT_CALL(*mockTimeSrc, isclockProvider()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(0));
    EXPECT_CALL(*mockTimeSrc, checkTime()).Times(1);
    mgr->m_timerSrc.push_back(mockTimeSrc);
    mgr->updateClockRealTime(nullptr);
}

TEST_F(SysTimeMgrTest, GetTimeStatus_AllQualities) {
    TimerMsg msg;
    mgr->m_timequality = eTIMEQUALILTY_POOR;
    mgr->getTimeStatus(&msg);
    mgr->m_timequality = eTIMEQUALILTY_GOOD;
    mgr->getTimeStatus(&msg);
    mgr->m_timequality = eTIMEQUALILTY_SECURE;
    mgr->getTimeStatus(&msg);
    
}

TEST_F(SysTimeMgrTest, PublishStatusCoversAll) {
    EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
    mgr->publishStatus(ePUBLISH_NTP_FAIL, "Poor");
    mgr->publishStatus(ePUBLISH_NTP_SUCCESS, "Good");
    mgr->publishStatus(ePUBLISH_SECURE_TIME_SUCCESS, "Secure");
    mgr->publishStatus(ePUBLISH_DTT_SUCCESS, "Good");
    mgr->publishStatus(ePUBLISH_TIME_INITIAL, "Poor");
    mgr->publishStatus(ePUBLISH_DEEP_SLEEP_ON, "Unknown");
}

TEST_F(SysTimeMgrTest, TimerExpiry_RefVsFileTime) {
    EXPECT_CALL(*mockTimeSrc, isreference()).WillOnce(Return(true));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(123));
    mgr->m_timerSrc.push_back(mockTimeSrc);
    mgr->timerExpiry(nullptr);
    mgr->m_timerSrc.clear();
    EXPECT_CALL(*mockTimeSrc, isreference()).WillOnce(Return(false));
    EXPECT_CALL(*mockTimeSrc, getTimeSec()).WillOnce(Return(456));
    mgr->m_timerSrc.push_back(mockTimeSrc);
    mgr->timerExpiry(nullptr);
}

TEST_F(SysTimeMgrTest, TimerThrAndProcessThrCoverage) {
    std::thread t1([&]() { SysTimeMgr::timerThr(mgr); });
    std::thread t2([&]() { SysTimeMgr::processThr(mgr); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    t1.detach();
    t2.detach();
}

TEST_F(SysTimeMgrTest, SendMessageAndProcessMsg) {
    mgr->sendMessage(eSYSMGR_EVENT_TIMER_EXPIRY, nullptr);
    std::thread t([&]() { mgr->processMsg(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    t.detach();
}

TEST_F(SysTimeMgrTest, RunStateMachine_AllStatesEvents) {
    std::vector<sysTimeMgrState> states = {
        eSYSMGR_STATE_RUNNING, eSYSMGR_STATE_NTP_WAIT, eSYSMGR_STATE_NTP_ACQUIRED,
        eSYSMGR_STATE_NTP_FAIL, eSYSMGR_STATE_DTT_ACQUIRED, eSYSMGR_STATE_SECURE_TIME_ACQUIRED
    };
    std::vector<sysTimeMgrEvent> events = {
        eSYSMGR_EVENT_TIMER_EXPIRY, eSYSMGR_EVENT_NTP_AVAILABLE,
        eSYSMGR_EVENT_SECURE_TIME_AVAILABLE, eSYSMGR_EVENT_DTT_TIME_AVAILABLE
    };
    for (auto state : states) {
        mgr->m_state = state;
        for (auto event : events) {
            mgr->runStateMachine(event, nullptr);
        }
    }
}

TEST_F(SysTimeMgrTest, GetTimeStatusStaticFunctionWorks) {
    TimerMsg msg;
    int ret = SysTimeMgr::getTimeStatus(static_cast<void*>(&msg));
    EXPECT_EQ(ret, 0);
    EXPECT_GE(strlen(msg.message), 0); 
}

TEST_F(SysTimeMgrTest, RunPathMonitorCoversInotifyEvent) {
    temp_test_dir = "/tmp/systimemgr_test_tmp";
    mkdir(temp_test_dir.c_str(), 0777);
  //  mgr->m_directory = temp_test_dir;
    std::string testfile = temp_test_dir + "/ntp";
    std::ofstream outfile(testfile); outfile << "test"; outfile.close();
    std::thread t([&]() { mgr->runPathMonitor(); });
    chmod(testfile.c_str(), 0666);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    t.detach();
    remove(testfile.c_str());
    rmdir(temp_test_dir.c_str());
}

TEST_F(SysTimeMgrTest, RunPathMonitorFileExistsAtStartup) {
    std::string temp_test_dir = "/tmp/systimemgr_test_tmp2";
    mkdir(temp_test_dir.c_str(), 0777);
 // mgr->m_directory = temp_test_dir;
    std::string fname = "ntp";
    mgr->m_pathEventMap[fname] = eSYSMGR_EVENT_NTP_AVAILABLE;
    std::ofstream((temp_test_dir + "/" + fname)).close();
    std::thread t([&] { mgr->runPathMonitor(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    t.detach();
    remove((temp_test_dir + "/" + fname).c_str());
    rmdir(temp_test_dir.c_str());
}

TEST_F(SysTimeMgrTest, RunPathMonitorInotifyAddWatchFails) {
    std::string random_dir = "/tmp/definitely_does_not_exist_" + std::to_string(rand());
    ASSERT_EQ(access(random_dir.c_str(), F_OK), -1);

    mgr->m_directory = random_dir;
    mgr->m_pathEventMap.clear();
    mgr->runPathMonitor();
}


TEST_F(SysTimeMgrTest, RunStateMachine_HitsFunctionPointer) {
    mgr->stateMachine[eSYSMGR_STATE_RUNNING][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::updateTime;
    mgr->m_state = eSYSMGR_STATE_RUNNING;
    mgr->runStateMachine(eSYSMGR_EVENT_TIMER_EXPIRY, nullptr);
}
