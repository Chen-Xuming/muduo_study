//
// Created by chen on 2022/11/6.
//

#include "EventLoopThread.h"

#include "EventLoop.h"

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const string &name)
        : loop_(nullptr),
          exiting_(false),
          thread_(std::bind(&EventLoopThread::threadFunc, this), name),
          mutex_(),
          cond_(mutex_),
          callback_(cb) {
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) // not 100% race-free, eg. threadFunc could be running callback_.
    {
        // still a tiny chance to call destructed object, if threadFunc exits just now.
        // but when EventLoopThread destructs, usually programming is exiting anyway.
        loop_->quit();
        thread_.join();
    }
}

/*
 *      TcpServer::start() --> EventLoopThreadPool::start() --> EventLoopThread()::startLoop();
 */
EventLoop *EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();

    EventLoop *loop = nullptr;
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait();               // notify in threadFunc()
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
    //assert(exiting_);
    MutexLockGuard lock(mutex_);
    loop_ = nullptr;
}