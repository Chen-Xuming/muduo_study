//
// Created by chen on 2022/11/6.
//

#include "TcpConnection.h"

#include "../base/Logging.h"
#include "../base/WeakCallback.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

/*
 *      默认的连接建立/关闭回调，什么也不做
 */
void muduo::net::defaultConnectionCallback(const TcpConnectionPtr &conn) {
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

/*
 *      默认的消息读取完毕回调。
 *      此时消息已经写到Buffer里面。这些只是将数据取出来。
 */
void muduo::net::defaultMessageCallback(const TcpConnectionPtr &,
                                        Buffer *buf,
                                        Timestamp) {
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
        : loop_(CHECK_NOTNULL(loop)),
          name_(nameArg),
          state_(kConnecting),
          reading_(true),
          socket_(new Socket(sockfd)),
          channel_(new Channel(loop, sockfd)),
          localAddr_(localAddr),
          peerAddr_(peerAddr),
          highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
    assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const {
    return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const {
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

void TcpConnection::send(const void *data, int len) {
    send(StringPiece(static_cast<const char *>(data), len));
}

/*
 *      send() --> sendInLoop()
 */
void TcpConnection::send(const StringPiece &message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                    std::bind(fp,
                              this,     // FIXME
                              message.as_string()));
            //std::forward<string>(message)));
        }
    }
}

/*
 *      send() --> sendInLoop()
 */
void TcpConnection::send(Buffer *buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        } else {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                    std::bind(fp,
                              this,     // FIXME
                              buf->retrieveAllAsString()));
            //std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece &message) {
    sendInLoop(message.data(), message.size());
}


void TcpConnection::sendInLoop(const void *data, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    /*
     *      a. 如果outputBuffer里面没有东西，那么会直接发送数据给对方。发送后有三种情况：
     *          1. 数据全部发送完了，则调用writeCompleteCallback_()
     *          2. 数据没有发送完，则把没发送的部分存到outputBuffer，然后监听writable事件，等到handwrite()里面再发送
     *          3. error
     *
     *      b. 如果outputBuffer里面有东西，为了不出现乱序，只好先存到outputBuffer再发送
     *
     */
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    /*
     *      将待发送数据存入outputBuffer。
     *      1. 如果存入后数据量超过设置的高水位，则触发highWaterMarkCallback_()
     *      2. 监听writable事件，然后以后的handlewrite()处理发送事件
     */
    assert(remaining <= len);
    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

/*
 *      shutdown() --> shutdownInLoop() --> socket_->shutdownWrite() 关闭写操作
 */
void TcpConnection::shutdown() {
    // FIXME: use compare and swap
    if (state_ == kConnected) {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose() {
    // FIXME: use compare and swap
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->runAfter(
                seconds,
                makeWeakCallback(shared_from_this(),
                                 &TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char *TcpConnection::stateToString() const {
    switch (state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading()) {
        channel_->disableReading();
        reading_ = false;
    }
}

/*
 *     当连接建立之后，监听该连接的可读事件
 *
 *     系统流程：
 *     TcpServer的acceptor_成员的 newConnectionCallback_ 绑定了 TcpServer::newConnection(),
 *     后者创建一个TcpConnection指针（TcpConnectionPtr对象），然后调用TcpConnection::connectEstablished()。
 *
 */
void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

/*
 *      TcpConnection::closeCallback_ 绑定的是 TcpServer::removeConnection(),
 *      后者会调用TcpConnection::connectDestroyed()
 */
void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

/*
 *      与 Channel::readCallback_绑定
 *      1. 读到数据则调用messageCallback_（用户给出）进行处理
 *      2. read() = 0则表示客户端已经关掉进程了，那么服务端也close()连接
 *      3. error
 */
void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

/*
 *      将outputBuffer的数据发送给客户端。与Channel_::writeCallback_绑定
 *      只有outputBuffer清空时才会触发writeCompleteCallback_()；否则poller会一直监听可写事件，handleWrite()一直被调用直到数据被发完。
 */
void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = sockets::write(channel_->fd(),
                                   outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    } else {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

/*
 *      TcpConnection关闭连接的流程
 *
 *      1. 对方关闭连接，触发TcpConnection::handleRead()，且read()返回0，进而调用TcpConnection::handleClose()
 *      2. TcpConnection::handleClose() 调用 closeCallback_()，后者绑定的是 TcpServer::removeConnection()
 *      3. TcpServer::removeConnection() --> TcpConnection::connectDestroyed() --> EventLoop::removeChannel() --> Poller:::removeChannel()
 *
 */
void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    // must be the last line
    closeCallback_(guardThis);
}

/*
 *      与Channel::errorCallback_绑定
 */
void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}