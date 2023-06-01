#ifndef ASYNC_LOGGING_H
#define ASYNC_LOGGING_H

#include "noncopyable.h"
#include "Thread.h"
#include "FixedBuffer.h"
#include "LogStream.h"
#include "LogFile.h"

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

/*
现在开始讲解 muduo 日志库的后端设计了。前端主要实现异步日志中的日志功能，
为用户提供将日志内容转换为字符串，封装为一条完整的 log 消息存放到 FixedBuffer 中；
而实现异步，核心是通过专门的后端线程，与前端线程并发运行，将 FixedBuffer 中的大
量日志消息写到磁盘上。

AsyncLogging 提供后端线程，定时将 log 缓冲写到磁盘，维护缓冲及缓冲队列。
LogFile 提供日志文件滚动功能，写文件功能。
AppendFile 封装了OS 提供的基础的写文件功能。
*/

class AsyncLogging
{
public:
    AsyncLogging(const std::string &basename,
                 off_t rollSize,
                 int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    // 前端调用 append 写入日志
    void append(const char *logling, int len);

    void start()
    {
        running_ = true;
        thread_.start();
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    // FixedBuffer缓冲区类型，而这个缓冲区大小由kLargeBuffer指定，
    // 大小为4M，因此，Buffer就是大小为4M的缓冲区类型。
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    void threadFunc();

    const int flushInterval_;   // 前端缓冲区定期向后端写入的时间（冲刷间隔）
    std::atomic<bool> running_; // 标识线程函数是否正在运行
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_; // 条件变量，主要用于前端缓冲区队列中没有数据时的休眠和唤醒

    BufferPtr currentBuffer_; // 当前缓冲区4M大小
    /**
     * 预备缓冲区，主要是在调用append向当前缓冲添加日志消息时，
     * 如果当前缓冲放不下，当前缓冲就会被移动到前端缓冲队列终端，
     * 此时预备缓冲区用来作为新的当前缓冲
     */
    BufferPtr nextBuffer_;
    BufferVector buffers_; // 前端缓冲区队列
};

#endif