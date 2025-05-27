#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <mutex>
#include <queue>
#include "systimemgr.h"
#include "systimemgr.cpp"

/ Prevent macro interference with enum
#ifdef TIMER_STATUS_MSG
#undef TIMER_STATUS_MSG
#endif

// Re-define enum in test scope (safe in test)
enum {
    TIMER_STATUS_MSG = 1
};

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// === Mock Interfaces ===

class MockTimerSrc : public ITimeSrc {
public:
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

class MockTimerSubscriber : public ITimerStatusSubscriber {
public:
    MOCK_METHOD(void, subscribe, (int, TimerStatusCallback), (override));
    MOCK_METHOD(void, notify, (const TimerStatus&), (override));
};

// === Test Fixture ===

class SysTimeMgrTest : public ::testing::Test {
protected:
    void SetUp() override {
        mgr = SysTimeMgr::getInstance();
    }

    void TearDown() override {
        // If needed, reset global state
    }

    SysTimeMgr* mgr;
};

// === Actual Test Case ===

TEST_F(SysTimeMgrTest, InitShouldSubscribeTimerStatus) {
    auto mockTimerSrc = std::make_shared<NiceMock<MockTimerSrc>>();
    auto mockTimerSubscriber = std::make_shared<NiceMock<MockTimerSubscriber>>();

    // Expect subscribe to be called once with TIMER_STATUS_MSG and any callback
    EXPECT_CALL(*mockTimerSubscriber, subscribe(TIMER_STATUS_MSG, _)).Times(1);

    // Call the init method
    mgr->init(mockTimerSrc, mockTimerSubscriber);
}
