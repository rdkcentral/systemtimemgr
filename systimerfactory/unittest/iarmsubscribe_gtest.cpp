#include <gtest/gtest.h>
#include "iarmsubscribe.h"
#include "iarmsubscribe.cpp"  // Assumes no header is available, otherwise include .h

class IarmSubscriberTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Reset static pointer after each test to avoid side effects
        IarmSubscriber::pInstance = nullptr;
    }
};

TEST_F(IarmSubscriberTest, Constructor_SetsStaticInstancePointer) {
    // Check that static pointer is initially null
    ASSERT_EQ(IarmSubscriber::pInstance, nullptr);

    IarmSubscriber subscriber("TestSubscriber");

    // Now static pointer should point to the created instance
    ASSERT_EQ(IarmSubscriber::pInstance, &subscriber);
}

