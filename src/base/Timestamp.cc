#include "Timestamp.h"
#include <sys/time.h>
using namespace std;

// 获取当前时间戳
Timestamp Timestamp::now()
{
    // 获取微妙和秒
    // 在x86-64平台gettimeofday()已不是系统调用,不会陷入内核, 多次调用不会有性能损失.
    struct timeval tv;
    int64_t seconds = tv.tv_sec;
    int64_t microseconds = tv.tv_usec;
    gettimeofday(&tv, NULL);
    return Timestamp(seconds * kMicroSecondsPerSecond + microseconds);
}

string Timestamp::toString() const
{
    char buf[128] = {0};
    // 使用localtime函数将秒数格式化成日历时间
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}

// 格式, "%4d年%02d月%02d日 星期%d %02d:%02d:%02d.%06d",时分秒.微秒
std::string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    // 使用localtime函数将秒数格式化成日历时间
    tm *tm_time = localtime(&seconds);
    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d.%06d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d/%02d/%02d %02d:%02d:%02d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec);
    }
    return buf;
}