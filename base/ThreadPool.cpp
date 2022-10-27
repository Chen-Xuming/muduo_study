//
// Created by chen on 2022/10/27.
//

#include "ThreadPool.h"

#include "Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string &nameArg)
        : mutex_(),
          notEmpty_(mutex_),
          notFull_(mutex_),
          name_(nameArg),
          maxQueueSize_(0),
          running_(false) {
}

ThreadPool::~ThreadPool() {
    if (running_) {
        stop();
    }
}

/*
 *      往线程池里添加numThreads个线程，然后启动它们
 *      每个线程初始化成功后调用“初始化完成回调函数”，然后不断从队列接任务、做任务
 */
void ThreadPool::start(int numThreads) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        char id[32];
        snprintf(id, sizeof id, "%d", i + 1);
        threads_.emplace_back(new muduo::Thread(
                std::bind(&ThreadPool::runInThread, this), name_ + id));

        /*
         *      start() ---> runInThread() ----> while(running_) {take task and process task}
         */
        threads_[i]->start();
    }
    if (numThreads == 0 && threadInitCallback_) {
        threadInitCallback_();
    }
}

/*
 *     注意：在让线程池结束工作（所有线程join）之前，应该唤醒所有被阻塞的线程，让它们正常运行完自己的函数
 */
void ThreadPool::stop() {
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        notEmpty_.notifyAll();
        notFull_.notifyAll();
    }
    for (auto &thr: threads_) {
        thr->join();
    }
}

// 返回任务队列中的任务个数
size_t ThreadPool::queueSize() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

/*
 *      往任务队列添加新任务。（感觉用run这个函数名不太好...）
 *      由于队列长度是有限制的，所以这个操作可能会被阻塞
 */
void ThreadPool::run(Task task) {
    if (threads_.empty()) {
        task();
    } else {
        MutexLockGuard lock(mutex_);
        while (isFull() && running_) {
            notFull_.wait();
        }
        if (!running_) return;
        assert(!isFull());

        queue_.push_back(std::move(task));
        notEmpty_.notify();
    }
}

ThreadPool::Task ThreadPool::take() {
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup  // 用while应对伪唤醒
    while (queue_.empty() && running_) {
        notEmpty_.wait();
    }
    Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0) {    // 这个判断条件多余的
            notFull_.notify();
        }
    }
    return task;
}

bool ThreadPool::isFull() const {
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

/*
 *
 *
 */
void ThreadPool::runInThread() {
    try {
        if (threadInitCallback_) {
            threadInitCallback_();
        }
        while (running_) {
            Task task(take());
            if (task) {
                task();
            }
        }
    }
    catch (const Exception &ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception &ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}
