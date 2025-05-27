#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "systimemgr.h"
#include "systimemgr.cpp"
#include "timerfactory.cpp"
#include "pubsubfactory.cpp"



class SysTimeMgrTest : public ::testing::Test {
protected:
    SysTimeMgr* mgr;
    void SetUp() override {
        mgr = SysTimeMgr::get_instance();
    }
};

TEST_F(SysTimeMgrTest, InitializeDoesNotThrow) {
    // Just verify that initialize() is callable and does not throw/crash
    ASSERT_NO_THROW(mgr->initialize());
}

TEST_F(SysTimeMgrTest, RunStateMachineAcceptsEvent) {
    // You can only test that it accepts input without observable side effects unless public state changes
    ASSERT_NO_THROW(mgr->runStateMachine(eSYSMGR_EVENT_TIMER_EXPIRY, nullptr));
}
