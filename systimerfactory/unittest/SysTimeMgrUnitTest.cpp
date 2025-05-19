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



