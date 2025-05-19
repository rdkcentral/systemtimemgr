#include <gtest/gtest.h>
#include <cstdio>
#include "timerfactory.h"
#include "ntptimesrc.h"
#include "stttimesrc.h"
#include "regulartimesrc.h"
#include "drmtimersrc.h"
#ifdef DTT_ENABLED
#include "dtttimersrc.h"
#endif
#include "testtimesync.h"
#include "rdkdefaulttimesync.h"
#ifdef TEE_ENABLED
#include "teetimesync.h"
#endif

// Print debug lines to the console, not using rdklogger
#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s] " format, #level, #module, ##__VA_ARGS__)

struct TimerFactoryFixture : public ::testing::Test
{
    void SetUp() override
    {
        printf("Setting up TimerFactoryFixture\n");
    }
    void TearDown() override
    {
        printf("Tearing down TimerFactoryFixture\n");
    }
};

TEST_F(TimerFactoryFixture, CreateNtpTimeSrc)
{
    printf("Test: CreateNtpTimeSrc\n");
    ITimeSrc* src = createTimeSrc("ntp", "");
    ASSERT_NE(src, nullptr);
    EXPECT_NE(dynamic_cast<NtpTimeSrc*>(src), nullptr);
    delete src;
}

TEST_F(TimerFactoryFixture, CreateSttTimeSrc)
{
    printf("Test: CreateSttTimeSrc\n");
    ITimeSrc* src = createTimeSrc("stt", "");
    ASSERT_NE(src, nullptr);
    EXPECT_NE(dynamic_cast<SttTimeSrc*>(src), nullptr);
    delete src;
}

TEST_F(TimerFactoryFixture, CreateRegularTimeSrc)
{
    printf("Test: CreateRegularTimeSrc\n");
    ITimeSrc* src = createTimeSrc("regular", "foo");
    ASSERT_NE(src, nullptr);
    EXPECT_NE(dynamic_cast<RegularTimeSrc*>(src), nullptr);
    delete src;
}

TEST_F(TimerFactoryFixture, CreateDrmTimeSrc)
{
    printf("Test: CreateDrmTimeSrc\n");
    ITimeSrc* src = createTimeSrc("drm", "bar");
    ASSERT_NE(src, nullptr);
    EXPECT_NE(dynamic_cast<DrmTimeSrc*>(src), nullptr);
    delete src;
}

#ifdef DTT_ENABLED
TEST_F(TimerFactoryFixture, CreateDttTimeSrc)
{
    printf("Test: CreateDttTimeSrc\n");
    ITimeSrc* src = createTimeSrc("dtt", "baz");
    ASSERT_NE(src, nullptr);
    EXPECT_NE(dynamic_cast<DttTimeSrc*>(src), nullptr);
    delete src;
}
#endif

TEST_F(TimerFactoryFixture, CreateTimeSrcInvalid)
{
    printf("Test: CreateTimeSrcInvalid\n

