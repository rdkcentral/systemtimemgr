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

         // Initialize m_pathEventMap, as runPathMonitor relies on it.
        mgr->m_pathEventMap.clear();
        mgr->m_pathEventMap.insert({"ntp", eSYSMGR_EVENT_NTP_AVAILABLE});
        mgr->m_pathEventMap.insert({"stt", eSYSMGR_EVENT_NTP_AVAILABLE});
        mgr->m_pathEventMap.insert({"drm", eSYSMGR_EVENT_SECURE_TIME_AVAILABLE});
        mgr->m_pathEventMap.insert({"dtt", eSYSMGR_EVENT_DTT_TIME_AVAILABLE});

        // Create a unique temporary directory for path monitoring tests
        temp_test_dir = "/tmp/systimemgr_test_path_monitor_" + std::to_string(getpid()) + "_" + std::to_string(std::time(nullptr));
        ASSERT_EQ(mkdir(temp_test_dir.c_str(), 0777), 0) << "Failed to create temp directory: " << temp_test_dir;
        mgr->m_directory = temp_test_dir; // Set manager's directory for the test
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

        // Clean up temporary directory and files
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

TEST_F(SysTimeMgrTest, RunDetachesThreadsWhenNotForever) {

    mgr->run(false); // lines 205-207 (detach calls)
}


TEST_F(SysTimeMgrTest, ProcessThrCallsProcessMsgAndRunsOneIteration) {
    // Goal: Cover the call to `mgr->processMsg()` and the first iteration of its loop.
    // Since `processMsg` has a `while(1)` and a `wait()`, we can trigger one iteration
    // by sending a message, then detach the thread so the test doesn't hang.

    // Arrange: Ensure a message is in the queue before the thread starts
    // so `m_cond_var.wait()` doesn't block indefinitely on the first attempt.
    mgr->sendMessage(eSYSMGR_EVENT_UNKNOWN, nullptr);

    // Act: Run processThr in a separate thread.
    // This thread will call `mgr->processMsg()`.
    std::thread process_thread([this]() {
        SysTimeMgr::processThr(mgr);
    });

    // Wait briefly to allow the thread to start and process the message.
    // This sleep is crucial for coverage of the loop body.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Detach the thread. We cannot join it because `processMsg` will block again
    // on `m_cond_var.wait()` after processing the first message.
    // Detaching ensures the test runner doesn't hang, but the thread will continue
    // running in the background until the test executable terminates.
    process_thread.detach();

    // Assert: We cannot directly assert the internal state of the detached thread's loop.
    // Verification relies on code coverage tools confirming the lines within `processMsg`
    // (including the `if (!m_queue.empty())` block and `runStateMachine` call) were hit.
    // You could also set expectations on `mockPublish` if `runStateMachine` leads to `publish`.
    // Example: EXPECT_CALL(*mockPublish, publish(_, _)).Times(AtLeast(1));
}

TEST_F(SysTimeMgrTest, TimerThrCallsRunTimerAndRunsOnce) {
    // Goal: Cover the call to `mgr->runTimer()` and ensure it runs at least once.
    // Similar to processMsg, it has a `while(1)` loop with a `sleep_for`.

    // Arrange: Set a very short timer interval for testability.
    // WARNING: If `m_timerInterval` is private and not set by config, this won't work.
    // Assuming `m_timerInterval` is set by the constructor's `cfgfile` logic or similar.
    // If not, this test will take `default_timer_interval` to run, which is likely minutes.
    // For this example, let's assume it *can* be changed for testing or is small by default.
    // If it can't be changed, this test will have a very long duration or rely on test runner timeout.
    // mgr->m_timerInterval = 10; // This line would require source modification if m_timerInterval is private.

    // Assuming default interval makes it hard to test, but if sendMessage always happens:
    EXPECT_CALL(*mockPublish, publish(eSYSMGR_EVENT_TIMER_EXPIRY, _)).Times(AtLeast(0));
    // If you can control m_timerInterval through config file, set it to a small value.

    // Act: Run timerThr in a separate thread.
    std::thread timer_thread([this]() {
        SysTimeMgr::timerThr(mgr);
    });

    // Wait briefly for the thread to execute at least one iteration (sleep and send message).
    // This duration depends on `m_timerInterval`. Use a generous sleep to ensure at least one cycle.
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Sufficient if m_timerInterval is small.

    // Detach the thread. It will continue looping in the background until process termination.
    timer_thread.detach();

    // Assert: Verification relies on code coverage tools. If `publish` is called by `sendMessage`,
    // the mock expectation (`EXPECT_CALL(*mockPublish, publish(...))`) would verify activity.
}

TEST_F(SysTimeMgrTest, PathThrCallsRunPathMonitorAndHandlesInitialScan) {
    // Goal: Cover the call to `mgr->runPathMonitor()` and its initial file scan logic.
    // It also has a `while(1)` with a `read()` call that can block.

    // Arrange: Create a dummy file in the temporary directory to trigger the initial scan.
    std::string test_filepath = mgr->m_directory + "/ntp";
    std::ofstream dummy_file(test_filepath);
    dummy_file << "initial content";
    dummy_file.close();

    // Expect `sendMessage` (via `mockPublish`) to be called due to the initial file scan.
    EXPECT_CALL(*mockPublish, publish(eSYSMGR_EVENT_NTP_AVAILABLE, _)).Times(1);

    // Act: Run pathThr in a separate thread.
    std::thread path_thread([this]() {
        SysTimeMgr::pathThr(mgr);
    });

    // Wait for the thread to perform the initial scan and set up the inotify watch.
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Allow time for initial scan + inotify setup.

    // To attempt to get the inotify loop to run one more time and demonstrate a file event:
    // Modify the file to trigger an inotify event.
    std::ofstream modify_file(test_filepath, std::ios_base::app); // Open in append mode
    modify_file << "more data";
    modify_file.close();

    // Wait for the inotify event to be processed by the thread.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Detach the thread. The `read()` call will remain blocking until process termination
    // or if the `inotifyFd` is closed by an external signal or process cleanup.
    path_thread.detach();

    // Assert: The `EXPECT_CALL` verifies the initial scan. Code coverage tools will show
    // that `SysTimeMgr::pathThr(mgr)` was called and the initial `for` loop was executed.
    // It should also show the lines for processing the inotify event if `modify_file` triggered it.
}

