#include "Logging.h"
#include "CurrentThread.h"

namespace ThreadInfo
{
    __thread char t_errnobuf[512];
    __thread char t_time[64];     // 线程时间
    __thread time_t t_lastSecond; // 线程最后一次时间调用
}

// 存储当前errno信息并返回errno
const char *getErrnoMsg(int savedErrno)
{
    //对于函数strerror_r，第一个参数errnum是错误代码，
    //第二个参数buf是用户提供的存储错误描述的缓存，第三个参数n是缓存的大小。
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf, sizeof(ThreadInfo::t_errnobuf));
}

// 根据Level返回Level名字
const char *getLevelName[Logger::LogLevel ::LEVEL_COUNT]{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// 初始日志等级
Logger::LogLevel initLogLevel()
{
    return Logger::INFO;
}

// 设置日志等级为初始等级INFO
Logger::LogLevel g_logLevel = initLogLevel();

/*
fread，fwrite>> CLib buffer内存缓冲(用户空间) -->fflush>>page  cache内核缓冲--->fsync>>磁盘

fflush：标准I/O函数(如：fread，fwrite)会在内存建立缓冲，该函数刷新内存缓冲，将内容写入
内核缓冲，要想将其写入磁盘，还需要调用fsync。(先调用fflush后调用fsync，否则不起作用)。

*/
// 默认输出到终端
static void defaultOutput(const char *data, int len)
{
    // stdout -- 标准输出流 -- 屏幕
    fwrite(data, len, sizeof(char), stdout);
}

// 默认刷新到终端
// 对标准输出流的清理，但是它并不是把数据丢掉，而是及时地打印数据到屏幕上。
// fflush(stdout)：清空输出缓冲区，并把缓冲区内容输出。
static void defaultFlush()
{
    fflush(stdout);
}

// 设置回调函数
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

// 日志的成员变量
Logger::Impl::Impl(Logger::LogLevel level, int savedErrno, const char *file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file)
{
    // 输出流 -> time
    formatTime();
    // 写入日志等级
    stream_ << GeneralTemplate(getLevelName[level], 6);
    // TODO:error
    if (savedErrno != 0)
    {
        stream_ << getErrnoMsg(savedErrno) << "(errno=" << savedErrno << ")";
    }
}

// Timestamp::toString方法的思路，只不过这里需要输出到流
void Logger::Impl::formatTime()
{
    Timestamp now = Timestamp::now();
    time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);

    struct tm *tm_time = localtime(&seconds);
    // 写入此线程存储的时间buf中
    snprintf(ThreadInfo::t_time, sizeof(ThreadInfo::t_time), "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    // 更新最后一次时间调用
    ThreadInfo::t_lastSecond = seconds;

    // muduo使用Fmt格式化整数，这里我们直接写入buf
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%06d", microseconds);

    // 线程输出时间，附有微秒(之前是(buf, 6),少了一个空格)
    // 年月日时分秒+微秒
    stream_ << GeneralTemplate(ThreadInfo::t_time, 17) << GeneralTemplate(buf, 7);
}

void Logger::Impl::finish()
{
    //-filename:行号
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_)
            << ':' << line_ << '\n';
}

// level默认为INFO等级
Logger::Logger(const char *file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(const char *file, int line, Logger::LogLevel level)
    : impl_(level, 0, file, line)
{
}

// 可以打印调用函数,func只会被打印，并无别的作用
Logger::Logger(const char *file, int line, Logger::LogLevel level, const char *func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}

Logger::~Logger()
{
    impl_.finish();
    // 获取buffer(stream_.buffer_)
    const LogStream::Buffer& buf(stream().buffer());
    // 输出(默认向终端输出)
    g_output(buf.data(), buf.length());
    // FATAL情况终止程序
    if(impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}


void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}
