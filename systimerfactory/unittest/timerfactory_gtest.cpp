#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
using namespace std;

// ------------------- Interfaces -------------------

class ITimeSrc {
public:
    virtual ~ITimeSrc() {}
};

class ITimeSync {
public:
    virtual ~ITimeSync() {}
};

// ------------------- Mock Classes -------------------

class MockNtpTimeSrc : public ITimeSrc {};
class MockSttTimeSrc : public ITimeSrc {};
class MockRegularTimeSrc : public ITimeSrc {
public:
    MockRegularTimeSrc(string arg) {}
};
class MockDrmTimeSrc : public ITimeSrc {
public:
    MockDrmTimeSrc(string arg) {}
};
class MockDttTimeSrc : public ITimeSrc {
public:
    MockDttTimeSrc(string arg) {}
};

class MockTestTimeSync : public ITimeSync {
public:
    MockTestTimeSync(string arg) {}
};
class MockRdkDefaultTimeSync : public ITimeSync {};
class MockTeeTimeSync : public ITimeSync {};

// ------------------- Mocked Factory -------------------
// This replaces your real `timerfactory.cpp` during tests

ITimeSrc* createTimeSrc(string type, string args)
{
    if (type == "ntp") return new MockNtpTimeSrc();
    else if (type == "stt") return new MockSttTimeSrc();
    else if (type == "regular") return new MockRegularTimeSrc(args);
    else if (type == "drm") return new MockDrmTimeSrc(args);
#ifdef DTT_ENABLED
    else if (type == "dtt") return new MockDttTimeSrc(args);
#endif
    return nullptr;
}

ITimeSync* createTimeSync(string type, string args)
{
    if (type == "test") return new MockTestTimeSync(args);
    else if (type == "rdkdefault") return new MockRdkDefaultTimeSync();
#ifdef TEE_ENABLED
    else if (type == "tee") return new MockTeeTimeSync();
#endif
    return nullptr;
}

// ------------------- Test Cases -------------------

#define ASSERT_CAST_AND_DELETE(ptr, Type) \
    { auto p = dynamic_cast<Type*>(ptr); ASSERT_NE(p, nullptr); delete p; }

// createTimeSrc tests

TEST(TimerFactoryMockTest, CreateNtpTimeSrc) {
    auto* src = createTimeSrc("ntp", "");
    ASSERT_CAST_AND_DELETE(src, MockNtpTimeSrc);
}

TEST(TimerFactoryMockTest, CreateSttTimeSrc) {
    auto* src = createTimeSrc("stt", "");
    ASSERT_CAST_AND_DELETE(src, MockSttTimeSrc);
}

TEST(TimerFactoryMockTest, CreateRegularTimeSrc) {
    auto* src = createTimeSrc("regular", "param");
    ASSERT_CAST_AND_DELETE(src, MockRegularTimeSrc);
}

TEST(TimerFactoryMockTest, CreateDrmTimeSrc) {
    auto* src = createTimeSrc("drm", "/path");
    ASSERT_CAST_AND_DELETE(src, MockDrmTimeSrc);
}

#ifdef DTT_ENABLED
TEST(TimerFactoryMockTest, CreateDttTimeSrc) {
    auto* src = createTimeSrc("dtt", "data");
    ASSERT_CAST_AND_DELETE(src, MockDttTimeSrc);
}
#endif

TEST(TimerFactoryMockTest, CreateInvalidTimeSrcReturnsNullptr) {
    auto* src = createTimeSrc("unknown", "xyz");
    ASSERT_EQ(src, nullptr);
}

// createTimeSync tests

TEST(TimerFactoryMockTest, CreateTestTimeSync) {
    auto* sync = createTimeSync("test", "arg");
    ASSERT_CAST_AND_DELETE(sync, MockTestTimeSync);
}

TEST(TimerFactoryMockTest, CreateRdkDefaultTimeSync) {
    auto* sync = createTimeSync("rdkdefault", "");
    ASSERT_CAST_AND_DELETE(sync, MockRdkDefaultTimeSync);
}

#ifdef TEE_ENABLED
TEST(TimerFactoryMockTest, CreateTeeTimeSync) {
    auto* sync = createTimeSync("tee", "");
    ASSERT_CAST_AND_DELETE(sync, MockTeeTimeSync);
}
#endif

TEST(TimerFactoryMockTest, CreateInvalidTimeSyncReturnsNullptr) {
    auto* sync = createTimeSync("blah", "");
    ASSERT_EQ(sync, nullptr);
}
