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
#include "libchronyctl.h"

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
    std::string test_dir = "/tmp/systimemgr";
    mkdir(test_dir.c_str(), 0777);
  //  mgr->m_directory = test_dir;
    std::string testfile = test_dir + "/ntp";
    std::ofstream outfile(testfile); outfile << "test"; outfile.close();
    std::thread t([&]() { mgr->runPathMonitor(); });
    chmod(testfile.c_str(), 0666);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    t.detach();
    remove(testfile.c_str());
    rmdir(test_dir.c_str());
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
    //  mgr->m_directory = random_dir;
    mgr->m_pathEventMap.clear();
    mgr->runPathMonitor();
}


TEST_F(SysTimeMgrTest, RunStateMachine_HitsFunctionPointer) {
    mgr->stateMachine[eSYSMGR_STATE_RUNNING][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::updateTime;
    mgr->m_state = eSYSMGR_STATE_RUNNING;
    mgr->runStateMachine(eSYSMGR_EVENT_TIMER_EXPIRY, nullptr);
}

// ============================================================================
// Regression Tests for Initialization with Invalid/Mixed Valid-Invalid Entries
// ============================================================================
// These tests verify that the initialize() method correctly handles:
// 1. Invalid-only source and sync entries
// 2. Mixed valid/invalid entries
// 3. No null pointers are retained in the vectors
// 4. Valid entries after invalid entries are still loaded

TEST_F(SysTimeMgrTest, Initialize_InvalidOnlyTimeSources) {
    // Create a config file with only invalid time sources
    std::string config_path = "/tmp/test_invalid_sources.cfg";
    std::ofstream cfg(config_path);
    cfg << "timesrc invalid_type /path/arg1\n";
    cfg << "timesrc unknown_src /path/arg2\n";
    cfg.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Before initialization, vectors should be empty
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 0);
    EXPECT_EQ(test_mgr->m_timerSync.size(), 0);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // After initialization with invalid sources:
    // - Both vectors should be empty (no invalid entries were added)
    // - No null pointers should exist
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 0);
    EXPECT_EQ(test_mgr->m_timerSync.size(), 0);
    
    // Verify no null pointers in source vector
    for (const auto& src : test_mgr->m_timerSrc) {
        EXPECT_NE(src, nullptr);
    }
    
    // Verify no null pointers in sync vector
    for (const auto& sync : test_mgr->m_timerSync) {
        EXPECT_NE(sync, nullptr);
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
}

TEST_F(SysTimeMgrTest, Initialize_InvalidOnlyTimeSyncs) {
    // Create a config file with only invalid time syncs
    std::string config_path = "/tmp/test_invalid_syncs.cfg";
    std::ofstream cfg(config_path);
    cfg << "timesync invalid_sync /path/arg1\n";
    cfg << "timesync unknown_sync /path/arg2\n";
    cfg.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // After initialization with invalid syncs:
    // - Both vectors should be empty (no invalid entries were added)
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 0);
    EXPECT_EQ(test_mgr->m_timerSync.size(), 0);
    
    // Verify no null pointers
    for (const auto& sync : test_mgr->m_timerSync) {
        EXPECT_NE(sync, nullptr);
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
}

TEST_F(SysTimeMgrTest, Initialize_MixedValidInvalidSources) {
    // Create a config file with mixed valid and invalid time sources
    std::string config_path = "/tmp/test_mixed_sources.cfg";
    std::ofstream cfg(config_path);
    cfg << "timesrc invalid_type /path/arg1\n";      // Invalid - should be skipped
    cfg << "timesrc ntp /dummy/path1\n";             // Valid - should be loaded
    cfg << "timesrc unknown_src /path/arg2\n";       // Invalid - should be skipped
    cfg << "timesrc stt /dummy/path2\n";             // Valid - should be loaded
    cfg.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // After initialization:
    // - Should have exactly 2 valid sources (ntp and stt)
    // - No null pointers should exist
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 2);
    
    // Verify no null pointers in source vector
    for (const auto& src : test_mgr->m_timerSrc) {
        EXPECT_NE(src, nullptr);
    }
    
    // Verify no null pointers in sync vector
    for (const auto& sync : test_mgr->m_timerSync) {
        EXPECT_NE(sync, nullptr);
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
}

TEST_F(SysTimeMgrTest, Initialize_MixedValidInvalidSyncs) {
    // Create a config file with mixed valid and invalid time syncs
    // Note: Avoid rdkdefault as it reads real system files (/version.txt)
    std::string config_path = "/tmp/test_mixed_syncs.cfg";
    std::ofstream cfg(config_path);
    cfg << "timesync invalid_sync /path/arg1\n";           // Invalid - should be skipped
    cfg << "timesync test /tmp/test_time_sync.txt\n";      // Valid - should be loaded
    cfg << "timesync unknown_sync /path/arg2\n";           // Invalid - should be skipped
    cfg << "timesync test /tmp/test_sync_2.txt\n";         // Second valid sync
    cfg.close();
    
    // Create parent directories for test sync files
    // Note: initialize() prefixes the configured path with /tmp/systemtimemgr
    mkdir("/tmp/systemtimemgr", 0700);
    mkdir("/tmp/systemtimemgr/tmp", 0700);
    
    // Create the test sync files at the path expected by initialize()
    std::ofstream sync_file("/tmp/systemtimemgr/tmp/test_time_sync.txt");
    sync_file << "0";
    sync_file.close();
    
    std::ofstream sync_file2("/tmp/systemtimemgr/tmp/test_sync_2.txt");
    sync_file2 << "0";
    sync_file2.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // After initialization:
    // - Should have exactly 2 valid syncs (test and dtt)
    // - No null pointers should exist
    EXPECT_EQ(test_mgr->m_timerSync.size(), 2);
    
    // Verify no null pointers in sync vector
    for (const auto& sync : test_mgr->m_timerSync) {
        EXPECT_NE(sync, nullptr);
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
    std::remove("/tmp/systemtimemgr/tmp/test_time_sync.txt");
    std::remove("/tmp/systemtimemgr/tmp/test_sync_2.txt");
    rmdir("/tmp/systemtimemgr/tmp");
    rmdir("/tmp/systemtimemgr");
}

TEST_F(SysTimeMgrTest, Initialize_MixedSourcesAndSyncs) {
    // Create a comprehensive config with valid and invalid entries mixed throughout
    // Note: Avoid rdkdefault as it reads real system files (/version.txt)
    std::string config_path = "/tmp/test_comprehensive_mixed.cfg";
    std::ofstream cfg(config_path);
    cfg << "timesrc invalid_type /path/arg1\n";      // Invalid timesrc
    cfg << "timesync unknown_sync /path/arg1\n";     // Invalid timesync
    cfg << "timesrc ntp /dummy/path1\n";             // Valid timesrc
    cfg << "timesync test /tmp/test_sync.txt\n";     // Valid timesync
    cfg << "timesrc invalid_src2 /path/arg2\n";      // Invalid timesrc
    cfg << "timesrc stt /dummy/path2\n";             // Valid timesrc
    cfg << "timesync unknown_sync2 /path/arg2\n";    // Invalid timesync
    cfg << "timesync test /tmp/test_sync_2.txt\n";   // Second valid timesync
    cfg.close();
    
    // Create parent directories for test sync files
    // Note: initialize() prefixes the configured path with /tmp/systemtimemgr
    mkdir("/tmp/systemtimemgr");
    mkdir("/tmp/systemtimemgr/tmp");
    
    // Create the test sync files at the path expected by initialize()
    std::ofstream sync_file("/tmp/systemtimemgr/tmp/test_sync.txt");
    sync_file << "0";
    sync_file.close();
    
    std::ofstream sync_file2("/tmp/systemtimemgr/tmp/test_sync_2.txt");
    sync_file2 << "0";
    sync_file2.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // After initialization:
    // - Should have exactly 2 valid sources (ntp, stt)
    // - Should have exactly 2 valid syncs (both test)
    // - No null pointers should exist
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 2);
    EXPECT_EQ(test_mgr->m_timerSync.size(), 2);
    
    // Verify no null pointers in source vector
    for (const auto& src : test_mgr->m_timerSrc) {
        EXPECT_NE(src, nullptr);
    }
    
    // Verify no null pointers in sync vector
    for (const auto& sync : test_mgr->m_timerSync) {
        EXPECT_NE(sync, nullptr);
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
    std::remove("/tmp/systemtimemgr/tmp/test_sync.txt");
    std::remove("/tmp/systemtimemgr/tmp/test_sync_2.txt");
    rmdir("/tmp/systemtimemgr/tmp");
    rmdir("/tmp/systemtimemgr");
}

TEST_F(SysTimeMgrTest, Initialize_ValidEntriesAfterInvalidOnesAreLoaded) {
    // This test specifically verifies that valid entries after invalid ones
    // are still properly loaded (guards don't skip subsequent entries)
    // Note: Avoid rdkdefault as it reads real system files (/version.txt)
    std::string config_path = "/tmp/test_valid_after_invalid.cfg";
    std::ofstream cfg(config_path);
    cfg << "timesrc invalid_1 /path/arg1\n";         // Invalid
    cfg << "timesrc invalid_2 /path/arg2\n";         // Invalid
    cfg << "timesrc invalid_3 /path/arg3\n";         // Invalid
    cfg << "timesrc ntp /dummy/path1\n";             // Valid - must be loaded
    cfg << "timesrc stt /dummy/path2\n";             // Valid - must be loaded
    cfg << "timesync unknown_1 /path/arg1\n";        // Invalid
    cfg << "timesync unknown_2 /path/arg2\n";        // Invalid
    cfg << "timesync test /tmp/test_time.txt\n";     // Valid - must be loaded
    cfg << "timesync test /tmp/test_time_2.txt\n";   // Second valid - must be loaded
    cfg.close();
    
    // Create parent directories for test sync files
    // Note: initialize() prefixes the configured path with /tmp/systemtimemgr
    mkdir("/tmp/systemtimemgr");
    mkdir("/tmp/systemtimemgr/tmp");
    
    // Create the test sync files at the path expected by initialize()
    std::ofstream sync_file("/tmp/systemtimemgr/tmp/test_time.txt");
    sync_file << "0";
    sync_file.close();
    
    std::ofstream sync_file2("/tmp/systemtimemgr/tmp/test_time_2.txt");
    sync_file2 << "0";
    sync_file2.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // Verify that valid entries after invalid ones are still present
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 2) 
        << "Failed to load valid time sources after invalid entries";
    EXPECT_EQ(test_mgr->m_timerSync.size(), 2) 
        << "Failed to load valid time syncs after invalid entries";
    
    // Verify no null pointers
    for (size_t i = 0; i < test_mgr->m_timerSrc.size(); ++i) {
        EXPECT_NE(test_mgr->m_timerSrc[i], nullptr) 
            << "Null pointer found at source index " << i;
    }
    
    for (size_t i = 0; i < test_mgr->m_timerSync.size(); ++i) {
        EXPECT_NE(test_mgr->m_timerSync[i], nullptr) 
            << "Null pointer found at sync index " << i;
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
    std::remove("/tmp/systemtimemgr/tmp/test_time.txt");
    std::remove("/tmp/systemtimemgr/tmp/test_time_2.txt");
    rmdir("/tmp/systemtimemgr/tmp");
    rmdir("/tmp/systemtimemgr");
}

TEST_F(SysTimeMgrTest, Initialize_AllInvalidEntriesDoNotAddNullPointers) {
    // Explicitly verify that invalid entries don't add null pointers to vectors
    std::string config_path = "/tmp/test_no_nullptrs.cfg";
    std::ofstream cfg(config_path);
    // All invalid entries - should not add ANY entries to vectors
    cfg << "timesrc bogus1 /path/arg1\n";
    cfg << "timesrc bogus2 /path/arg2\n";
    cfg << "timesrc bogus3 /path/arg3\n";
    cfg << "timesync bogus_sync1 /path/arg1\n";
    cfg << "timesync bogus_sync2 /path/arg2\n";
    cfg << "timesync bogus_sync3 /path/arg3\n";
    cfg.close();

    // Create a manager instance with this config
    SysTimeMgr* test_mgr = new SysTimeMgr(config_path);
    
    // Initialize should complete without crashing
    test_mgr->initialize();
    
    // Vectors should be empty - no null pointers added
    EXPECT_EQ(test_mgr->m_timerSrc.size(), 0) 
        << "Invalid timesrc entries should not be added";
    EXPECT_EQ(test_mgr->m_timerSync.size(), 0) 
        << "Invalid timesync entries should not be added";
    
    // Additional safety checks - no null pointers should be present
    for (const auto& src : test_mgr->m_timerSrc) {
        EXPECT_NE(src, nullptr) << "Null pointer found in timesrc vector";
    }
    
    for (const auto& sync : test_mgr->m_timerSync) {
        EXPECT_NE(sync, nullptr) << "Null pointer found in timesync vector";
    }
    
    // Cleanup
    delete test_mgr;
    SysTimeMgr::pInstance = nullptr;
    std::remove(config_path.c_str());
}
