//
// Created by chen on 2022/10/26.
//

#ifndef MYMUDUO_ASYNCLOGGING_H
#define MYMUDUO_ASYNCLOGGING_H

#include "BlockingQueue.h"
#include "BoundedBlockingQueue.h"
#include "CountDownLatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "LogStream.h"

#include <atomic>
#include <vector>

/*
 *      muduo多线程异步日志 书本P114
 *
 *      1. 每个进程只写一个日志文件。用一个背景线程负责收集日志消息，并写入日志文件；
 *         其它业务线程只管往这个“日志线程”发送日志消息。
 *
 *      2. 前端调用  AsyncLogging::append() 添加日志即可（准确来说是发送一条日志消息），而不用关心后端如何、何时写入文件。
 *      3. 后端则由线程函数 AsyncLogging::threadFunc() 将前端buffers转移到后端buffers，然后写入文件。
 */

namespace muduo {

    class AsyncLogging : noncopyable {
    public:

        AsyncLogging(const string &basename,
                     off_t rollSize,
                     int flushInterval = 3);

        ~AsyncLogging() {
            if (running_) {
                stop();
            }
        }

        void append(const char *logline, int len);

        void start() {
            running_ = true;
            thread_.start();
            latch_.wait();
        }

        void stop()
        {
            running_ = false;
            cond_.notify();
            thread_.join();
        }

    private:

        void threadFunc();

        typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
        typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
        typedef BufferVector::value_type BufferPtr;

        const int flushInterval_;
        std::atomic<bool> running_;
        const string basename_;
        const off_t rollSize_;
        muduo::Thread thread_;
        muduo::CountDownLatch latch_;
        muduo::MutexLock mutex_;
        muduo::Condition cond_;
        BufferPtr currentBuffer_;
        BufferPtr nextBuffer_;
        BufferVector buffers_;
    };

}  // namespace muduo

#endif //MYMUDUO_ASYNCLOGGING_H
