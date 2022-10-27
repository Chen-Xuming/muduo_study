//
// Created by chen on 2022/10/21.
//

#include "Condition.h"
#include <errno.h>

bool muduo::Condition::waitForSeconds(double seconds) {
    // 获取当前系统时间（精确度为纳秒）
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);

    // 计算过seconds秒之后的时间（即绝对时间，pthread_cond_timedwait要求使用绝对时间）
    const int64_t kNanoSecondsPerSecond = 1000 * 1000 * 1000;   // 1s = 10^9 ns
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
    abstime.tv_sec += static_cast<time_t>((abstime.tv_sec + nanoseconds) / kNanoSecondsPerSecond);
    abstime.tv_nsec = static_cast<long>((abstime.tv_sec + nanoseconds) % kNanoSecondsPerSecond);

    MutexLock::UnassignGuard ug(mutex_);
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}
