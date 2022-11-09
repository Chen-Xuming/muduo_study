//
// Created by chen on 2022/11/6.
//

#include "Connector.h"

#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop),
          serverAddr_(serverAddr),
          connect_(false),
          state_(kDisconnected),
          retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!channel_);
}

/*
 *      start() --> startInLoop() --> connect()
 */
void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

/*
 *      stop() --> stopInLoop()
 */
void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
    // FIXME: cancel timer
}

/*
 *      当处于正在连接状态（kConnecting），重置channel然后重试
 *      （既然stop，为什么还要重连？）
 */
void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

/*
 *      跟据sockets::connect()返回值，分为三类处理：
 *
 *      1. 连接成功 / 连接正在进行： connecting() 继续监听新连接 / 正在进行的连接
 *      2. EAGAIN，对方拒绝连接等：retry() 重连，并且重连时间间隔每次延长
 *      3. 一些比较严重的Error： close() 关闭socket
 */
void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:                     // 成功
        case EINPROGRESS:           // Operation now in progress
        case EINTR:                 // Interrupted system call
        case EISCONN:               // Transport endpoint is already connected
            connecting(sockfd);
            break;

        case EAGAIN:                // Resource temporarily unavailable
        case EADDRINUSE:            // Address already in use
        case EADDRNOTAVAIL:         // Cannot assign requested address
        case ECONNREFUSED:          // Connection refused
        case ENETUNREACH:           // Network is unreachable
            retry(sockfd);
            break;

        case EACCES:                // Permission denied
        case EPERM:                 // Operation not permitted
        case EAFNOSUPPORT:          // Address family not supported by protocol
        case EALREADY:              // Operation already in progress
        case EBADF:                 // Bad file descriptor
        case EFAULT:                // Bad address
        case ENOTSOCK:              // Socket operation on non-socket
            LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

/*
 *      restart() --> startInloop()
 */
void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

/*
 *      连接成功/连接正在进行， reset channel 继续监听可写事件
 *
 *      Q: 为什么要reste?
 *      A: Acceptor是可以反复使用的，但是socket是一次性的，因此每次尝试连接都要使用新的socket和新的Channel对象
 */
void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(
            std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
    channel_->setErrorCallback(
            std::bind(&Connector::handleError, this)); // FIXME: unsafe

    // channel_->tie(shared_from_this()); is not working,
    // as channel_ is not managed by shared_ptr
    channel_->enableWriting();
}

/*
 *      1. 到Poller注销channel_
 *      2. reset channel_ (因为Poller不负责管理Channel的生命周期)
 */
int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

/*
 *       注意：handleWrite的触发并不意味着连接成功建立，需要通过::getsockopt()再次确认
 *
 */
void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << state_;

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror_tl(err);
            retry(sockfd);
        }
        /*
         *      检查是否自连接
         */
        else if (sockets::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }
}

/*
 *      重新连接
 */
void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
                 << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_/1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);              // 每次重连间隔翻倍，最多为30s
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}