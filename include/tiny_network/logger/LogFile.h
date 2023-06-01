#ifndef LOG_FILE_H
#define LOG_FILE_H

#include "FileUtil.h"

#include <mutex>
#include <memory>

/*
LogFile 主要职责：提供对日志文件的操作，包括滚动日志文件、将 log 数据写到当前
 log 文件、flush log数据到当前 log 文件。 写日志文件操作 LogFile提供了2个接口，
 用于向当前日志文件file_写入数据。append本质上是通过append_unlocked完成对
 日志文件写操作，但多了线程安全。用户只需调用第一个接口即可，append会根据
 线程安全需求，自行判断是否需要加上；第二个是private接口。
*/
class LogFile
{
public:
    LogFile(const std::string &basename,
            off_t rollSize,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char *data, int len);
    void flush();
    bool rollFile(); // 滚动日志

private:
    static std::string getLogFileName(const std::string &basename, time_t *now);
    void appendInLock(const char *data, int len);

    const std::string basename_; // 日志文件basename
    const off_t rollSize_;       // 日志文件达到rolsize生成一个新文件
    const int flushInterval_;    // 日志写入间隔时间
    const int checkEveryN_;      // 每调用checkEveryN_次日志写，就滚动一次日志

    int count_; // 写入的次数

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_; // 开始记录日志时间
    time_t lastRoll_;      // 上一次滚动日志文件时间
    time_t lastFlush_;     // 上一次日志写入文件时间
    std::unique_ptr<FileUtil> file_;    //文件智能指针

    const static int kRollPerSeconds_ = 60 * 60 * 24;   //即时间一天
};

#endif