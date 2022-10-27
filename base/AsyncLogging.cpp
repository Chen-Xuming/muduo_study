//
// Created by chen on 2022/10/26.
//

#include "AsyncLogging.h"
#include "LogFile.h"
#include "Timestamp.h"

#include <stdio.h>
#include <functional>

using namespace muduo;

AsyncLogging::AsyncLogging(const string &basename,
                           off_t rollSize,
                           int flushInterval)
        : flushInterval_(flushInterval),
          running_(false),
          basename_(basename),
          rollSize_(rollSize),
          thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
          latch_(1),
          mutex_(),
          cond_(mutex_),
          currentBuffer_(new Buffer),
          nextBuffer_(new Buffer),
          buffers_() {
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

/*
 *      前端维护两个buffer和一个bufferVector
 *
 *      1. append()整个函数体都是加锁的，同一时间只能由一个工作线程添加一条日志
 *      2. 当一个buffer被写满，则将其移到bufferVector里面等待后端线程取走
 *      3. 先写满currentBuffer，再写满nextBuffer，如果二者都写满了而后端还未就绪，则申请新的buffer
 *      4. currentBuffer写满时通知后端线程（cond_.notify()）
 *      5. 即便currentBuffer未写满，后端线程也会将其移到BufferVector，并取走
 *
 */
void AsyncLogging::append(const char *logline, int len) {
    muduo::MutexLockGuard lock(mutex_);
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline, len);
    } else {
        buffers_.push_back(std::move(currentBuffer_));

        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            currentBuffer_.reset(new Buffer); // Rarely happens
        }
        currentBuffer_->append(logline, len);
        cond_.notify();
    }
}

/*
 *      后端线程也维护两个Buffer（newBuffer1/2）和一个BufferVector（BuffersToWrite）：
 *      两个Buffer是空闲的，用于置换前端的两个Buffer；BufferVector包含着所有要写入文件的Buffer
 *
 *
 *
 */
void AsyncLogging::threadFunc() {
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_, rollSize_, false);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        /*
         *      前后端的交互主要完成两个事件：
         *      1. 将buffers_的内容转移到buffersToWrite
         *      2. 将两个备用buffer替代前端的两个buffer
         *
         *      后续的写入文件和前端无关，所以只需要在这部分加锁
         */
        {
            muduo::MutexLockGuard lock(mutex_);
            /*
             *  如果日志写入很频繁，那么cond_.notify()是有可能丢失的：即cond_.waitForSeconds()之后的代码在运行时，前端currentBuffer写满后又调用了cond_.notify()。
             *  这里用if而不是while的原因正是为了应对这种情况————当while(running_)下一轮迭代运行到此处发现buffers_不空，还是会把待处理的日志写到文件中
             *  见书本P119 情况4
             */
            if (buffers_.empty())  // unusual usage!
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_) {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        /// ------------------------ 将日志写入文件，重置备用两个buffer -------------------------

        assert(!buffersToWrite.empty());

        /*
         *      如果buffer过多了，那么判定很可能出现了异常（前端死循环不断发送日志）
         *      这里的处理方法是直接丢弃这些日志（只保留前两个）
         */
        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (const auto &buffer: buffersToWrite) {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffer->data(), buffer->length());
        }

        if (buffersToWrite.size() > 2) {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}