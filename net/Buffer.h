//
// Created by chen on 2022/11/6.
//

/*
 *      Muduo Buffer类的设计见 P209
 */

#ifndef MYMUDUO_BUFFER_H
#define MYMUDUO_BUFFER_H

#include "../base/copyable.h"
#include "../base/StringPiece.h"
#include "../base/Types.h"

#include "Endian.h"

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>

namespace muduo {
    namespace net {

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
        class Buffer : public muduo::copyable {
        public:
            static const size_t kCheapPrepend = 8;          // 初始化prependable大小
            static const size_t kInitialSize = 1024;        // 初始化writable大小

            explicit Buffer(size_t initialSize = kInitialSize)
                    : buffer_(kCheapPrepend + initialSize),
                      readerIndex_(kCheapPrepend),
                      writerIndex_(kCheapPrepend) {
                assert(readableBytes() == 0);
                assert(writableBytes() == initialSize);
                assert(prependableBytes() == kCheapPrepend);
            }

            // implicit copy-ctor, move-ctor, dtor and assignment are fine
            // NOTE: implicit move-ctor is added in g++ 4.6

            void swap(Buffer &rhs) {
                buffer_.swap(rhs.buffer_);
                std::swap(readerIndex_, rhs.readerIndex_);
                std::swap(writerIndex_, rhs.writerIndex_);
            }

            size_t readableBytes() const { return writerIndex_ - readerIndex_; }

            size_t writableBytes() const { return buffer_.size() - writerIndex_; }          // 注意size() 和 capacity() 的区别

            size_t prependableBytes() const { return readerIndex_; }

            const char *peek() const { return begin() + readerIndex_; }     // readerIndex指向的位置

            // 在可读数据范围内查找 \r\n 开始的位置
            const char *findCRLF() const {
                // FIXME: replace with memmem()?
                const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
                return crlf == beginWrite() ? nullptr : crlf;
            }

            // 在可读数据范围内，从给定位置start开始查找 \r\n 开始的位置
            const char *findCRLF(const char *start) const {
                assert(peek() <= start);
                assert(start <= beginWrite());
                // FIXME: replace with memmem()?
                const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
                return crlf == beginWrite() ? nullptr : crlf;
            }

            // 在可读数据范围内查找 \n 的位置
            const char *findEOL() const {
                const void *eol = memchr(peek(), '\n', readableBytes());
                return static_cast<const char *>(eol);
            }

            // 在可读数据范围内，从给定位置start开始查找 \n 的位置
            const char *findEOL(const char *start) const {
                assert(peek() <= start);
                assert(start <= beginWrite());
                const void *eol = memchr(start, '\n', beginWrite() - start);
                return static_cast<const char *>(eol);
            }

            // retrieve returns void, to prevent
            // string str(retrieve(readableBytes()), readableBytes());
            // the evaluation of two functions are unspecified
            /*
             *      读出长度为len的数据
             *      如果可读数据不足len，那么读出全部数据
             *      注：没有返回值，该函数仅仅是设置readerIndex_而已
             */
            void retrieve(size_t len) {
                assert(len <= readableBytes());
                if (len < readableBytes()) {
                    readerIndex_ += len;
                } else {
                    retrieveAll();
                }
            }

            /*
             *      读出 peek() 到 end 的数据
             */
            void retrieveUntil(const char *end) {
                assert(peek() <= end);
                assert(end <= beginWrite());
                retrieve(end - peek());
            }

            void retrieveInt64() {
                retrieve(sizeof(int64_t));
            }

            void retrieveInt32() {
                retrieve(sizeof(int32_t));
            }

            void retrieveInt16() {
                retrieve(sizeof(int16_t));
            }

            void retrieveInt8() {
                retrieve(sizeof(int8_t));
            }

            /*
             *     当将所有可读数据取出来后，复位 readerIndex_ 和 writerIndex
             */
            void retrieveAll() {
                readerIndex_ = kCheapPrepend;
                writerIndex_ = kCheapPrepend;
            }

            // 将所有可读数据以string形式读出
            string retrieveAllAsString() {
                return retrieveAsString(readableBytes());
            }

            // 将长度为len的可读数据以string形式读出
            string retrieveAsString(size_t len) {
                assert(len <= readableBytes());
                string result(peek(), len);
                retrieve(len);
                return result;
            }

            // 返回可读数据的StringPiece形式
            StringPiece toStringPiece() const {
                return StringPiece(peek(), static_cast<int>(readableBytes()));
            }

            // 将StringPiece写入Buffer
            void append(const StringPiece &str) {
                append(str.data(), str.size());
            }
            void append(const char * /*restrict*/ data, size_t len) {
                ensureWritableBytes(len);
                std::copy(data, data + len, beginWrite());
                hasWritten(len);
            }
            void append(const void * /*restrict*/ data, size_t len) {
                append(static_cast<const char *>(data), len);
            }

            /*
             *  确保可写空间大小足够
             */
            void ensureWritableBytes(size_t len) {
                if (writableBytes() < len) {
                    makeSpace(len);
                }
                assert(writableBytes() >= len);
            }

            char *beginWrite() { return begin() + writerIndex_; }

            const char *beginWrite() const { return begin() + writerIndex_; }

            /*
             *   writerIndex后移表示已写入
             */
            void hasWritten(size_t len) {
                assert(len <= writableBytes());
                writerIndex_ += len;
            }

            /*
             *   writeIndex前移表示未写入
             */
            void unwrite(size_t len) {
                assert(len <= readableBytes());
                writerIndex_ -= len;
            }

            ///
            /// Append int64_t using network endian
            ///
            void appendInt64(int64_t x) {
                int64_t be64 = sockets::hostToNetwork64(x);
                append(&be64, sizeof be64);
            }

            ///
            /// Append int32_t using network endian
            ///
            void appendInt32(int32_t x) {
                int32_t be32 = sockets::hostToNetwork32(x);
                append(&be32, sizeof be32);
            }

            void appendInt16(int16_t x) {
                int16_t be16 = sockets::hostToNetwork16(x);
                append(&be16, sizeof be16);
            }

            void appendInt8(int8_t x) {
                append(&x, sizeof x);
            }

            ///
            /// Read int64_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int64_t readInt64() {
                int64_t result = peekInt64();
                retrieveInt64();
                return result;
            }

            ///
            /// Read int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int32_t readInt32() {
                int32_t result = peekInt32();
                retrieveInt32();
                return result;
            }

            int16_t readInt16() {
                int16_t result = peekInt16();
                retrieveInt16();
                return result;
            }

            int8_t readInt8() {
                int8_t result = peekInt8();
                retrieveInt8();
                return result;
            }

            ///
            /// Peek int64_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int64_t)
            /*
             *      从头部读取Int64(网络字节序)，然后转换并返回Host字节序
             */
            int64_t peekInt64() const {
                assert(readableBytes() >= sizeof(int64_t));
                int64_t be64 = 0;
                ::memcpy(&be64, peek(), sizeof be64);
                return sockets::networkToHost64(be64);
            }

            ///
            /// Peek int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int32_t peekInt32() const {
                assert(readableBytes() >= sizeof(int32_t));
                int32_t be32 = 0;
                ::memcpy(&be32, peek(), sizeof be32);
                return sockets::networkToHost32(be32);
            }

            int16_t peekInt16() const {
                assert(readableBytes() >= sizeof(int16_t));
                int16_t be16 = 0;
                ::memcpy(&be16, peek(), sizeof be16);
                return sockets::networkToHost16(be16);
            }

            int8_t peekInt8() const {
                assert(readableBytes() >= sizeof(int8_t));
                int8_t x = *peek();
                return x;
            }

            ///
            /// Prepend int64_t using network endian
            ///
            void prependInt64(int64_t x) {
                int64_t be64 = sockets::hostToNetwork64(x);
                prepend(&be64, sizeof be64);
            }

            ///
            /// Prepend int32_t using network endian
            ///
            void prependInt32(int32_t x) {
                int32_t be32 = sockets::hostToNetwork32(x);
                prepend(&be32, sizeof be32);
            }

            void prependInt16(int16_t x) {
                int16_t be16 = sockets::hostToNetwork16(x);
                prepend(&be16, sizeof be16);
            }

            void prependInt8(int8_t x) {
                prepend(&x, sizeof x);
            }

            void prepend(const void * /*restrict*/ data, size_t len) {
                assert(len <= prependableBytes());
                readerIndex_ -= len;
                const char *d = static_cast<const char *>(data);
                std::copy(d, d + len, begin() + readerIndex_);
            }

            /*
             *      通过创建新的Buffer然后swap来缩小capacity
             *      因为同一个Buffer的capacity是只增不减的
             */
            void shrink(size_t reserve) {
                // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
                Buffer other;
                other.ensureWritableBytes(readableBytes() + reserve);
                other.append(toStringPiece());
                swap(other);
            }

            size_t internalCapacity() const {
                return buffer_.capacity();
            }

            /// Read data directly into buffer.
            ///
            /// It may implement with readv(2)
            /// @return result of read(2), @c errno is saved
            ssize_t readFd(int fd, int *savedErrno);

        private:

            char *begin() { return &*buffer_.begin(); }

            const char *begin() const { return &*buffer_.begin(); }

            /*
             *      通过扩容或者数据腾挪，使得可写入的空间满足要求
             */
            void makeSpace(size_t len) {
                /*
                 *      如果writable+prependable大小不够（即无法通过数据腾挪的方法满足要求），那么直接扩容
                 */
                if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
                    // FIXME: move readable data
                    buffer_.resize(writerIndex_ + len);
                }

                /*
                 *      数据腾挪（通常发生在readerIndex_比较靠后的情况）
                 */
                else {
                    // move readable data to the front, make space inside buffer
                    assert(kCheapPrepend < readerIndex_);
                    size_t readable = readableBytes();
                    std::copy(begin() + readerIndex_,
                              begin() + writerIndex_,
                              begin() + kCheapPrepend);
                    readerIndex_ = kCheapPrepend;
                    writerIndex_ = readerIndex_ + readable;
                    assert(readable == readableBytes());
                }
            }

        private:
            std::vector<char> buffer_;
            size_t readerIndex_;
            size_t writerIndex_;

            static const char kCRLF[];      // \r\n
        };

    }  // namespace net
}  // namespace muduo

#endif //MYMUDUO_BUFFER_H
