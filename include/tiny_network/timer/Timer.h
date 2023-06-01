#ifndef TIMER_H
#define TIMER_H

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>

/*
    Timer
    一个定时器需要哪些属性呢？

    定时器到期后需要调用回调函数
    我们需要让定时器记录我们设置的超时时间
    如果是重复事件（比如每间隔5秒扫描一次用户连接），我们还需要记录超时时间间隔
    对应的，我们需要一个 bool 类型的值标注这个定时器是一次性的还是重复的
*/
class Timer : noncopyable
{
public:
    // 回调函数
    using TimerCallback = std::function<void()>;

    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0) // 一次性定时器设置为0
    {
    }

    void run() const
    {
        callback_();
    }

    //获取下一次超时时刻
    Timestamp expiration() const { return expiration_; }
    //定时器是否重复
    bool repeat() const {return repeat_;}

    // 重启定时器(如果是非重复定时事件则到期时间置为0)
    void restart(Timestamp now);

private:
    const TimerCallback callback_; // 定时器回调函数
    Timestamp expiration_;         // 下一次的超时时刻
    const double interval_;        // 超时时间间隔，如果是一次性定时器，该值为0
    const bool repeat_;            // 是否重复(false 表示是一次性定时器)
};

#endif