//
// Created by chen on 2022/11/6.
//

#include "Buffer.h"

#include "SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;


/*
 *      从fd读取数据到Buffer
 *
 *      这里用了两块内存：a.buffer_的可写部分; b. 一块64K大小的额外空间
 *      这样设计的好处：
 *      1. 当内存块a大小足够时，直接读到内存a就行了；当readv读的数据超过了内存a的大小，才把多的部分读到内存b，然后append到buffer_，此时buffer_扩容。
 *         好处：buffer_不需要一开始就设得很大，浪费空间。
 *      2. readv()不一定只调用一次，而是可能多次调用才把数据读完。如果不开辟临时得额外空间，buffer_又可能太小，那么readv()调用多次的可能性变大，性能低。
 *         为readv()设置足够空间后，通常一次调用就能读完数据。
 *
 */
ssize_t Buffer::readFd(int fd, int *savedErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    } else if (implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}
