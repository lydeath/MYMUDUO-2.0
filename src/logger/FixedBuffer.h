#ifndef FIXED_BUFFER_H
#define FIXED_BUFFER_H

#include "noncopyable.h"
#include <assert.h>
#include <string.h> // memcpy
#include <strings.h>
#include <string>
/*
之前谈到了实现异步日志需要先将前端消息储存起来，然后到达一定数量或者一定时间再将
这些信息写入磁盘。而 muduo 使用 FixedBuffer 类来实现这个存放日志信息的缓冲区。
FixedBuffer 的实现在 LogStream.h 文件中。 针对不同的缓冲区，muduo 设置了两个固定容量。
*/

const int kSmallBuffer = 4000;        // 4KB
const int kLargeBuffer = 4000 * 1000; // 4MB

/*
LogStream 类用于重载正文信息，一次信息大小是有限的，其使用的缓冲区的大小就是 kSmallBuffer。
而后端日志 AsyncLogging 需要存储大量日志信息，其会使用的缓冲区大小更大，所以是 kLargeBuffer。
*/
template <int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer()
        : cur_(data_)
    {
    }

    void append(const char *buf, size_t len)
    {
        // 如果可用大小>数据大小
        if (static_cast<size_t>(avail()) > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    // 获得data
    const char *data() const {return data_;}
    // 获得data首地址到结尾的长度
    int length() const { return static_cast<int>(end() - data_); }
    // 获得当前可用位置
    char *current() { return cur_; }
    // 可用大小
    int avail() const { return static_cast<int>(end() - cur_); }
    // 更新cur
    void add(size_t len){cur_ += len;}
    // 重新设置data
    void reset() { cur_ = data_; }
    // 置0
    void bzero() { ::bzero(data_, sizeof(data_)); }
    //将data转换为字符串
    std::string toString() const { return std::string(data_, length()); }

private:
    const char *end() const { return data_ + sizeof(data_); }

    char data_[SIZE];
    char *cur_;
};

#endif