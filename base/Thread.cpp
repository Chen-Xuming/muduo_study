//
// Created by chen on 2022/10/26.
//

#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include "Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo {
    namespace detail {

        void afterFork() {
            muduo::CurrentThread::t_cachedTid = 0;
            muduo::CurrentThread::t_threadName = "main";
            CurrentThread::tid();
            // no need to call pthread_atfork(NULL, NULL, &afterFork);
        }

        class ThreadNameInitializer {
        public:
            ThreadNameInitializer() {
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                pthread_atfork(nullptr, nullptr, &afterFork);
            }
        };

        ThreadNameInitializer init;

        struct ThreadData {
            typedef muduo::Thread::ThreadFunc ThreadFunc;
            ThreadFunc func_;
            string name_;
            pid_t *tid_;
            CountDownLatch *latch_;

            ThreadData(ThreadFunc func,
                       const string &name,
                       pid_t *tid,
                       CountDownLatch *latch)
                    : func_(std::move(func)),
                      name_(name),
                      tid_(tid),
                      latch_(latch) {}

            void runInThread() {
                *tid_ = muduo::CurrentThread::tid();
                tid_ = nullptr;
                latch_->countDown();    //
                latch_ = nullptr;

                muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
                ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
                try {
                    func_();
                    muduo::CurrentThread::t_threadName = "finished";
                }
                catch (const Exception &ex) {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
                    abort();
                }
                catch (const std::exception &ex) {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    abort();
                }
                catch (...) {
                    muduo::CurrentThread::t_threadName = "crashed";
                    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                    throw; // rethrow
                }
            }
        };

        void *startThread(void *obj) {
            ThreadData *data = static_cast<ThreadData *>(obj);
            data->runInThread();
            delete data;
            return nullptr;
        }

    }  // namespace detail


    AtomicInt32 Thread::numCreated_;

    Thread::Thread(ThreadFunc func, const string &n)
            : started_(false),
              joined_(false),
              pthreadId_(0),
              tid_(0),
              func_(std::move(func)),
              name_(n),
              latch_(1) {
        setDefaultName();
    }

    Thread::~Thread() {
        if (started_ && !joined_) {
            pthread_detach(pthreadId_);
        }
    }

    void Thread::setDefaultName() {
        int num = numCreated_.incrementAndGet();
        if (name_.empty()) {
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", num);
            name_ = buf;
        }
    }

    /*
     *      Thread::start() --->  Muduo::detail::startThread(data)      // ThreadData *data
     *                      --->  data.runInThread()    // 触发latch.countDown() 通知主线程子线程已初始化完毕
     *                      --->  func()                // 实际线程函数
     *
     */
    void Thread::start() {
        assert(!started_);
        started_ = true;
        // FIXME: move(func_)
        detail::ThreadData *data = new detail::ThreadData(func_, name_, &tid_, &latch_);
        if (pthread_create(&pthreadId_, nullptr, &detail::startThread, data)) {     // pthread_create成功返回0，失败返回错误码
            started_ = false;
            delete data; // or no delete?
            LOG_SYSFATAL << "Failed in pthread_create";
        } else {
            latch_.wait();  // runInThread() 会调用 latch_.countDown(), 目的是通知主线程：线程初始化已完成
            assert(tid_ > 0);
        }
    }

    int Thread::join() {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        return pthread_join(pthreadId_, nullptr);
    }

}  // namespace muduo