#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "iarmpublish.h"
#include "iarmpublish.cpp"

// Mocking IARM Bus functions
extern "C" {
    MOCK_METHOD(int, IARM_Bus_IsConnected, (const char*, int*), ());
    MOCK_METHOD(int, IARM_Bus_Init, (const char*), ());
    MOCK_METHOD(int, IARM_Bus_Connect, (), ());
    MOCK_METHOD(int, IARM_Bus_BroadcastEvent, (const char*, int, void*, int), ());
}

// Helper to inject mocks
class IarmPublishTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing specific needed for setup
    }

    void TearDown() override {
        // Clean up if needed
    }
};

// Struct to match TimerMsg layout used in publish()
struct TimerMsg {
    int event;
    int quality;
    char message[256];
};

TEST_F(IarmPublishTest, Constructor_WhenNotConnected_InitializesAndConnects) {
    const char* expectedPublisher = "TestPublisher";

    EXPECT_CALL(::testing::MockFunction<int(const char*, int*)>(), Call(expectedPublisher, ::testing::_))
        .WillOnce([](const char*, int* registered) {
            *registered = 0;
            return 1;  // Simulate not connected
        });

    EXPECT_CALL(::testing::MockFunction<int(const char*)>(), Call(expectedPublisher)).Times(1); // IARM_Bus_Init
    EXPECT_CALL(::testing::MockFunction<int()>(), Call()).Times(1); // IARM_Bus_Connect

    IarmPublish publisher(expectedPublisher);
}

TEST_F(IarmPublishTest, Publish_BroadcastsCorrectly) {
    const char* expectedPublisher = "TestPublisher";

    // Constructor expectations
    EXPECT_CALL(::testing::MockFunction<int(const char*, int*)>(), Call(::testing::_, ::testing::_))
        .WillOnce([](const char*, int* registered) {
            *registered = 1;
            return 0;  // Already connected
        });

    IarmPublish publisher(expectedPublisher);

    TimerMsg msg;
    msg.event = 42;
    msg.quality = 100;
    strcpy(msg.message, "Timer Expired");

    EXPECT_CALL(::testing::MockFunction<int(const char*, int, void*, int)>(), 
        Call(expectedPublisher, 42, ::testing::_, sizeof(TimerMsg)))
        .Times(1);

    publisher.publish(42, &msg);
}

