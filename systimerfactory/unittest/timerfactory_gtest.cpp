



#include "Client_Mock.h"

#define _IRDKLOG_H_
#define RDK_LOG(level, module, format, ...) printf("[%s:%s]" format, #level, #module, __VA_ARGS__)
// Only for unit test
#define NTP_TIME_LOG_FMT "[%s:%d]:NTP Time Values, MaxValue = %d, Time in Sec = %ld, Time in Microsec = %ld, Estimated Error = %d, TAI = %ld \n"


#include "timerfactory.h"
#include "timerfactory.cpp"
#include "drmtimersrc.cpp"
#include "rdkdefaulttimesync.cpp"
using namespace std;
using namespace testing;




// ------------------- Test Cases -------------------

#define ASSERT_CAST_AND_DELETE(ptr, Type) \
    { auto p = dynamic_cast<Type*>(ptr); ASSERT_NE(p, nullptr); delete p; }

// createTimeSrc tests

TEST(TimerFactoryMockTest, CreateNtpTimeSrc) {
    ITimeSrc* src = createTimeSrc("ntp", "");
    ASSERT_CAST_AND_DELETE(src, NtpTimeSrc);
}

TEST(TimerFactoryMockTest, CreateSttTimeSrc) {
    ITimeSrc* src = createTimeSrc("stt", "");
    ASSERT_CAST_AND_DELETE(src, SttTimeSrc);
}

TEST(TimerFactoryMockTest, CreateRegularTimeSrc) {
    ITimeSrc* src = createTimeSrc("regular", "param");
    ASSERT_CAST_AND_DELETE(src, RegularTimeSrc);
}

TEST(TimerFactoryMockTest, CreateDrmTimeSrc) {
    ITimeSrc* src = createTimeSrc("drm", "/path");
    ASSERT_CAST_AND_DELETE(src, DrmTimeSrc);
}

/*#ifdef DTT_ENABLED
TEST(TimerFactoryMockTest, CreateDttTimeSrc) {
    ITimeSrc* src = createTimeSrc("dtt", "data");
    ASSERT_CAST_AND_DELETE(src, DttTimeSrc);
}
#endif*/

TEST(TimerFactoryMockTest, CreateInvalidTimeSrcReturnsNullptr) {
    ITimeSrc* src = createTimeSrc("unknown", "xyz");
    ASSERT_EQ(src, nullptr);
}

// createTimeSync tests

TEST(TimerFactoryMockTest, CreateTestTimeSync) {
    ITimeSync* sync = createTimeSync("test", "arg");
    ASSERT_CAST_AND_DELETE(sync, TestTimeSync);
}

TEST(TimerFactoryMockTest, CreateRdkDefaultTimeSync) {
    ITimeSync* sync = createTimeSync("rdkdefault", "");
    ASSERT_CAST_AND_DELETE(sync, RdkDefaultTimeSync);
}

/*#ifdef TEE_ENABLED
TEST(TimerFactoryMockTest, CreateTeeTimeSync) {
    ITimeSync* sync = createTimeSync("tee", "");
    ASSERT_CAST_AND_DELETE(sync, TeeTimeSync);
}
#endif*/

TEST(TimerFactoryMockTest, CreateInvalidTimeSync) {
    ITimeSync* sync = createTimeSync("blah", "");
    ASSERT_EQ(sync, nullptr);
}
int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
