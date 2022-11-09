//
// Created by chen on 2022/11/6.
//

#include "Acceptor.h"

#include "../base/Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
        : loop_(loop),
          acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
          acceptChannel_(loop, acceptSocket_.fd()),
          listening_(false),
          idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))        // 见handleRead()
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
            std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
    else
    {
        LOG_SYSERR << "in Acceptor::handleRead";

        /*
         *      EMFILE：文件描述符已经耗尽
         *      既然没有一个fd能够用来表示这个连接，那么说明也无法close()这个连接。而listening socket上将会一直监听到连接，
         *      handleRead()也会一直触发，系统CPU占用率飙升但其实什么也没做。
         *
         *      解决方法：
         *      在Acceptor初始化的时候，向系统申请一个文件描述符，用来“占坑”：
         *          idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
         *      当accept()返回EMFILE错误的时候，通过close(idleFd_)来让出一个文件描述符，随后accept()一个连接，它将占用这个文件描述符。
         *      此时就可以通过close()把这个连接关掉。随后又通过上述占坑的方式申请这个文件描述符，应对listening队列中的其它连接。
         *
         *      书本P238
         *
         */
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}