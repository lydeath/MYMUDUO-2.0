#include "Timer.h"

void Timer::restart(Timestamp now)
{
    if (repeat_)    //如果是重复定时事件
    {
        /*
        如果是重复使用的定时器，在触发任务之后还需重新利用。这里就会调用 restart 方法。
        我们设置其下一次超时时间为「当前时间 + 间隔时间」。如果是「一次性定时器」，那么
        就会自动生成一个空的 Timestamp，其时间自动设置为 0.0。
        */
        //设置下一次超时时刻
        //继续添加定时事件，得到新事件到期事件
        expiration_ = addTime(now, interval_);
    }
    else
    {
        //置为0可以再触发一次
        expiration_ = Timestamp();
    }
}