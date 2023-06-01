#include "TimerQueue.h"
#include "Timestamp.h"
#include "Timer.h"
#include "EventLoop.h"
#include "Logging.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

/*
整个TimerQueue只使用一个timerfd来观察定时事件，并且每次重置timerfd时
只需跟set中第一个节点比较即可，因为set本身是一个有序队列。
整个定时器队列采用了muduo典型的事件分发机制，可以使的定时器的到期时
间像fd一样在Loop线程中处理。
之前Timestamp用于比较大小的重载方法在这里得到了很好的应用
*/

int createTimerfd()
{
    /**
     * CLOCK_MONOTONIC：绝对时间
     * TFD_NONBLOCK：非阻塞
     */
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_ERROR << "Failed in timerfd_create";
    }
    return timerfd;
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop_, timerfd_),
      timers_() // 定时器队列
{
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    // 删除所有定时器
    for (const Entry &timer : timers_)
    {
        delete timer.second;
    }
}

// 插入定时器（回调函数，到期时间，是否重复）
void TimerQueue::addTimer(TimerCallback cb,
                          Timestamp when,
                          double interval)
{
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    // 是否取代了最早的定时触发时间
    bool eraliestChanged = insert(timer);

    // 我们需要重新设置timerfd_触发时间
    if (eraliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 重置timerfd
void TimerQueue::resetTimerfd(int timerfd_, Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, '\0', sizeof(newValue));
    memset(&oldValue, '\0', sizeof(oldValue));

    // 超时时间 - 现在时间
    int64_t microSecondDif = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microSecondDif < 100)
    {
        microSecondDif = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microSecondDif / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microSecondDif % Timestamp::kMicroSecondsPerSecond) * 1000);
    newValue.it_value = ts;
    // 此函数会唤醒事件循环
    /*
    timerfd_settime()	      用来启动或关闭fd指定的定时器
    fd	                                       timerfd_create函数返回的定时器文件描述符timerfd
    flags	                                1代表设置的是绝对时间；为0代表相对时间
    new_value	                   指定新的超时时间，设定new_value.it_value非零则启动定时器，为零关闭定时器，如果new_value.it_interval为0，则定时器只定时一次，即初始那次，否则之后每隔设定时间it_interval超时一次
    old_value	                     不为null，则返回定时器这次设置之前的超时时间

    */
    if (::timerfd_settime(timerfd_, 0, &newValue, &oldValue))
    {
        LOG_ERROR << "timerfd_settime faield()";
    }
}

// 类外函数要放在类内函数之前，不然会编译错误
void ReadTimerFd(int timerfd)
{
    uint64_t read_byte;
    ssize_t readn = ::read(timerfd, &read_byte, sizeof(read_byte));

    if (readn != sizeof(read_byte))
    {
        LOG_ERROR << "TimerQueue::ReadTimerFd read_size < 0";
    }
}

/*
*******************
当epoll检测到timerfd_文件描述符可读，即timerfdChannel_可读时，此时
定时器到时，会调用handleRead函数，这是所有定时器处理器的公共入口。
*******************

EventLoop获取活跃的activeChannel，并分别调取它们绑定的回调函数。
这里对于timerfd_，就是调用了handleRead方法。
handleRead方法获取已经超时的定时器组数组，遍历到期的定时器并调用
内部绑定的回调函数。之后调用reset方法重新设置定时器
reset方法内部判断这些定时器是否是可重复使用的，如果是则继续插入定
时器管理队列，之后自然会触发事件。如果不是，那么就删除。如果重新
插入了定时器，记得还需重置timerfd_。
*/
// 定时器读事件触发的函数
void TimerQueue::handleRead()
{
    Timestamp now = Timestamp::now();

    ReadTimerFd(timerfd_);

    // 获取到期的定时器
    std::vector<Entry> expired = getExpired(now);

    // 遍历到期的定时器，调用回调函数
    callingExpiredTimers_ = true;
    for (const Entry &it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;

    // 重新设置这些定时器
    reset(expired, now);
}

// 移除所有已到期的定时器
// 1.获取到期的定时器
// 2.重置这些定时器（销毁或者重复定时任务）
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;

    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX)); // 创建一个时间戳和定时器地址的集合
    TimerList::iterator end = timers_.lower_bound(sentry);     // 二分查找
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;
    for (const Entry &it : expired)
    {
        // 重复任务则继续执行
        if (it.second->repeat())
        {
            auto timer = it.second;
            timer->restart(Timestamp::now());
            insert(timer); // 插入timer_定时器列表*****
        }
        else // 非重复任务则删除定时器
        {
            delete it.second;
        }

        // 如果重新插入了定时器，需要继续重置timerfd
        if (!timers_.empty())
        {
            resetTimerfd(timerfd_, (timers_.begin()->second)->expiration());
        }
    }
}

// 插入定时器的内部方法
bool TimerQueue::insert(Timer *timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    // 如果定时器列表为空或插入定时器过期时间最早
    if (it == timers_.end() || when < it->first)
    {
        // 说明最早的定时器已经被替换了
        earliestChanged = true;
    }

    // 定时器管理红黑树插入此新定时器
    timers_.insert(Entry(when, timer));

    return earliestChanged;
}
