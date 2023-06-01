#include "LogFile.h"

LogFile::LogFile(const std::string &basename,
                 off_t rollSize,
                 int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(new std::mutex),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
    rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char *data, int len)
{
    std::lock_guard<std::mutex> lock(*mutex_);
    appendInLock(data, len);
}

void LogFile::appendInLock(const char *data, int len)
{
    // 写入数据
    file_->append(data, len);

    // 当文件写入数据大小到达限制，滚动日志
    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else // 若尚未到达限制
    {
        ++count_;
        if (count_ >= checkEveryN_) // 每调用checkEveryN_次日志写，就滚动一次日志
        {
            count_ = 0;
            time_t now = ::time(NULL);
            // 注意，这里先除KRollPerSeconds然后乘KPollPerSeconds表示对齐值KRollPerSeconds的
            // 整数倍，也就是事件调整到当天零点(/除法会引发取整)
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod != startOfPeriod_) // 如果还是当天
            {
                rollFile(); // 滚动日志
            }
            // 当前时间-上次刷新时间 > 刷新间隔  ，刷新缓存池写入文件
            else if (now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

void LogFile::flush()
{
    // std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
}

// 滚动日志
// basename + time + hostname + pid + ".log"
bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now); // 获取生成一个文件名称
    // 计算现在是第几天 now/kRollPerSeconds求出现在是第几天，
    // 再乘以秒数相当于是当前天数0点对应的秒数
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    //
    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ - now;
        startOfPeriod_ = start;
        // 让file_指向一个名为filename的文件，相当于新建了一个文件
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}

std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    /*
    localtime、localtime_r 二者区别：
    localtime对于多线程不安全，因为 localtime在使用时，只需定义一个指针，
    申请空间的动作由函数自己完成，这样在多线程的情况下，如果有另一个线程
    调用了这个函数，那么指针指向的struct tm结构体的数据就会改变。

    在localtime_s与localtime_r调用时，定义的是struct tm的结构体，获取到的时
    间已经保存在struct tm中，并不会受其他线程的影响。
    */
    localtime_r(now, &tm);
    // 写入时间
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";

    return filename;
}
