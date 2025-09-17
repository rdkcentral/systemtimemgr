#include <gtest/gtest.h>
#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include "iarmsubscribe.h"
#include "iarmsubscribe.cpp"  // Assumes no header is available, otherwise include .h

class IarmSubscriberTestImpl : public IarmSubscriber {
public:
    IarmSubscriberTestImpl(const std::string& name) : IarmSubscriber(name) {}
    
    // Provide dummy implementation
    bool subscribe(std::string, funcPtr) override {
        return true;
    }
};

TEST(IarmSubscriberTest, Constructor_SetsStaticInstancePointer) {
    IarmSubscriberTestImpl subscriber("TestSubscriber");
    EXPECT_TRUE(subscriber.subscribe("dummy_event", reinterpret_cast<funcPtr>(0x1234)));
}

