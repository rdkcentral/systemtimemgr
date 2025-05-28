#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Client_Mock.h"
//#include "secure_wrapper.h"
#include "systimemgr.h"
#include "systimemgr.cpp"
#include "timerfactory.cpp"
#include "pubsubfactory.cpp"
#include "rdkdefaulttimesync.cpp"
#include "drmtimersrc.cpp"








class SysTimeMgrTest : public ::testing::Test {
protected:
    SysTimeMgr* mgr;
    void SetUp() override {
        mgr = SysTimeMgr::get_instance();
        ASSERT_NE(mgr, nullptr) << "SysTimeMgr::get_instance() returned nullptr";
    }
    void TearDown() override {
        // Not deleting singleton to avoid double-free/static issues.
    }
};

// Test 1: Singleton can be constructed
TEST_F(SysTimeMgrTest, SingletonConstructs) {
    EXPECT_NE(mgr, nullptr);
}

// Test 2: Initialize does not throw or crash
TEST_F(SysTimeMgrTest, InitializeDoesNotThrow) {
    EXPECT_NO_THROW(mgr->initialize());
}

// Test 3: State machine can accept a timer expiry event
TEST_F(SysTimeMgrTest, RunStateMachineAcceptsTimerExpiry) {
    mgr->initialize();
    EXPECT_NO_THROW(mgr->runStateMachine(eSYSMGR_EVENT_TIMER_EXPIRY, nullptr));
}

// Test 4: State machine can accept a secure time event
TEST_F(SysTimeMgrTest, RunStateMachineAcceptsSecureTime) {
    mgr->initialize();
    EXPECT_NO_THROW(mgr->runStateMachine(eSYSMGR_EVENT_SECURE_TIME_AVAILABLE, nullptr));
}
