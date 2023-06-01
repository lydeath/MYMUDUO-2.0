#pragma once

#include <functional>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "TimerQueue.h"

class Channel;
class Poller;

// 事件循环类  主要包含了两个大模块 Channel   Poller（epoll的抽象）
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp poolReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    /**
     * 把cb放入队列，唤醒loop所在的线程执行cb
     *
     * 实例情况：
     * 在mainLoop中获取subLoop指针，然后调用相应函数
     * 在queueLoop中发现当前的线程不是创建这个subLoop的线程，
     * 将此函数装入subLoop的pendingFunctors容器中
     * 之后mainLoop线程会调用subLoop::wakeup向subLoop的
     * eventFd写数据，以此唤醒subLoop来执行pengdingFunctors
     */
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在的线程的
    void wakeup();

    // EventLoop的方法 =》 Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断运行当前EventLoop的线程是否是当前使用的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    /**
     * 定时任务相关函数
     */
    void runAt(Timestamp timestamp, Functor&& cb){
        //添加非重复时间并立刻执行
        timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }

    //添加非重复事件并在waitTime后执行
    void runAfter(double waitTime, Functor&& cb){
        Timestamp time(addTime(Timestamp::now(), waitTime));
        runAt(time, std::move(cb));
    }

    //添加重复事件且间隔为interval，并在interval时长后执行
    void runEvery(double interval, Functor&& cb){
        Timestamp timestamp(addTime(Timestamp::now(), interval));
        timerQueue_->addTimer(std::move(cb), timestamp, interval);
    }

private:
    void handleRead();        // wake up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; // 原子操作，通过CAS实现的
    std::atomic_bool quit_;    // 标识退出loop循环

    const pid_t threadId_; // 记录当前loop所在线程的id

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点

    std::unique_ptr<Poller> poller_;
    // 主要作用，当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_; // 当前处理的活跃channel

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有的回调操作

    std::mutex mutex_; // 互斥锁，用来保护上面vector容器的线程安全操作

    std::unique_ptr<TimerQueue> timerQueue_;
};