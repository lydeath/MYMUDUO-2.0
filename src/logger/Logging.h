#ifndef LOGGING_H
#define LOGGING_H

#include "Timestamp.h"
#include "LogStream.h"
#include <string.h>
#include <functional>

// SourceFile的作用是提取文件名
class SourceFile
{
public:
    explicit SourceFile(const char *filename)
        : data_(filename)
    {
        /**
         * 找出data中出现/最后一次的位置，从而获取具体的文件名
         * 2022/10/26/test.log
         */
        const char *slash = strrchr(filename, '/');
        if (slash)
        {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }

    const char *data_;
    int size_;
};

class Logger
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        LEVEL_COUNT,
    };

    // member function
    Logger(const char *file, int line);
    Logger(const char *file, int line, LogLevel level);
    Logger(const char *file, int line, LogLevel level, const char *func);
    ~Logger();

    // 流是会改变的
    LogStream &stream() { return impl_.stream_; }
    static LogLevel logLevel();     //*************
    static void setLogLevel(LogLevel level);

    // 输出函数和刷新缓冲区函数
    using OutputFunc = std::function<void(const char *msg, int len)>;
    static void setOutput(OutputFunc);
    using FlushFunc = std::function<void()>;
    static void setFlush(FlushFunc);

private:
    // 内部类
    class Impl
    {
    public:
        using LogLevel = Logger::LogLevel;
        Impl(LogLevel level, int savedErrno, const char *file, int line);
        void formatTime();
        void finish();

        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };

    // Logger's member variable
    Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel logLevel()
{
    return g_logLevel;
}

// 获取当前线程errno信息
const char *getErrnoMsg(int savedErrno);

/*
观察宏函数，会发现每个宏定义都会构造出一个 Logger 的临时对象，然后输出相关信息。
在 Logger::Impl 的构造函数中会初始化时间戳、线程ID、日志等级这类固定信息，而正文
消息靠 LogStream 重载实现。在 Logger 临时对象销毁时，会调用 Logger 的析构函数，
其内部会将日志信息输出到指定位置。 而 Logger 的实现文件中定义了两个全局函数指针，
其执行的函数会确定日志信息的输出位置。
*/

/**
 * 当日志等级小于对应等级才会输出
 * 比如设置等级为FATAL，则logLevel等级大于DEBUG和INFO，DEBUG和INFO等级的日志就不会输出
 */
// 当前日志等级小于DEBUG，输出
#define LOG_DEBUG                    \
    if (logLevel() <= Logger::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
// 当前日志等级小于INFO，输出
#define LOG_INFO                    \
    if (logLevel() <= Logger::INFO) \
    Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()

#endif // LOGGING_H