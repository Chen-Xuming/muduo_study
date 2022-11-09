//
// Created by chen on 2022/11/6.
//

#ifndef MYMUDUO_EVENTLOOPTHREAD_H
#define MYMUDUO_EVENTLOOPTHREAD_H

#include "../base/Condition.h"
#include "../base/Mutex.h"
#include "../base/Thread.h"

namespace muduo {
    namespace net {

        class EventLoop;

        class EventLoopThread : noncopyable {
        public:
            typedef std::function<void(EventLoop *)> ThreadInitCallback;

            EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                            const string &name = string());

            ~EventLoopThread();

            EventLoop *startLoop();

        private:
            void threadFunc();

            EventLoop *loop_;
            bool exiting_;
            Thread thread_;
            MutexLock mutex_;
            Condition cond_;
            ThreadInitCallback callback_;
        };

    }  // namespace net
}  // namespace muduo


#endif //MYMUDUO_EVENTLOOPTHREAD_H
