#include <gtest/gtest.h>
#include "timerfactory.h"
#include "testpublish.h"
#include "testsubscribe.h"

#ifdef ENABLE_IARM
#include "iarmpublish.h"
#include "iarmsubscribe.h"
#include "iarmtimerstatussubscriber.h"
#ifdef PWRMGRPLUGIN_ENABLED
#include "ipowercontrollersubscriber.h"
#else
#include "iarmpowersubscriber.h"
#endif
#endif

#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
#include "pubsubfactory.cpp"

#define ASSERT_CAST_AND_DELETE(ptr, Type) \
    { auto p = dynamic_cast<Type*>(ptr); ASSERT_NE(p, nullptr); delete p; }

// ---------------------- Test createPublish ------------------------

TEST(TimerFactoryPubSubTest, CreateTestPublish) {
    IPublish* pub = createPublish("test", "args_for_test");
    ASSERT_CAST_AND_DELETE(pub, TestPublish);
}

#ifdef ENABLE_IARM
TEST(TimerFactoryPubSubTest, CreateIarmPublish) {
    IPublish* pub = createPublish("iarm", "args_for_iarm");
    ASSERT_CAST_AND_DELETE(pub, IarmPublish);
}
#endif

TEST(TimerFactoryPubSubTest, CreateInvalidPublishReturnsNullptr) {
    IPublish* pub = createPublish("unknown", "some_arg");
    ASSERT_EQ(pub, nullptr);
}

// ---------------------- Test createSubscriber ---------------------

TEST(TimerFactoryPubSubTest, CreateTestSubscriber) {
    ISubscribe* sub = createSubscriber("test", "args_test", "unused_subtype");
    ASSERT_CAST_AND_DELETE(sub, TestSubscriber);
}

#ifdef ENABLE_IARM
TEST(TimerFactoryPubSubTest, CreateIarmTimerStatusSubscriber) {
    ISubscribe* sub = createSubscriber("iarm", "args_timer", TIMER_STATUS_MSG);
    ASSERT_CAST_AND_DELETE(sub, IarmTimerStatusSubscriber);
}

#ifdef PWRMGRPLUGIN_ENABLED
TEST(TimerFactoryPubSubTest, CreateIpowerControllerSubscriber) {
    ISubscribe* sub = createSubscriber("iarm", "args_power", POWER_CHANGE_MSG);
    ASSERT_CAST_AND_DELETE(sub, IpowerControllerSubscriber);
}
#else
TEST(TimerFactoryPubSubTest, CreateIarmPowerSubscriber) {
    ISubscribe* sub = createSubscriber("iarm", "args_power", POWER_CHANGE_MSG);
    ASSERT_CAST_AND_DELETE(sub, IarmPowerSubscriber);
}
#endif
#endif

TEST(TimerFactoryPubSubTest, CreateInvalidSubscriberReturnsNullptr) {
    ISubscribe* sub = createSubscriber("unknown", "some_arg", "some_subtype");
    ASSERT_EQ(sub, nullptr);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
