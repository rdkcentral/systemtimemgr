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
#define v_secure_system(cmd) \
    do { \
        printf("[SECURE_SYSTEM] Executing: %s\n", cmd); \
        system(cmd); \
    } while(0)






extern "C" int v_secure_system(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[2048];

    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    std::string command = buffer;
    printf("command: %s\n", command.c_str());
    return system(command.c_str());
}

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
