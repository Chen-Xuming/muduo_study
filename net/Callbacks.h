//
// Created by chen on 2022/11/1.
//

/*
 *      声明了几种回调函数：
 *      1. 定时器回调
 *      2. 连接建立回调
 *      3. 连接断开回调
 *      4. 写事件完成回调
 *      5. 收到消息后回调
 *      6. 高水位警告回调
 *
 */

#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include "../base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo {

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    // should really belong to base/Types.h, but <memory> is not included there.

    template<typename T>
    inline T *get_pointer(const std::shared_ptr<T> &ptr) {
        return ptr.get();
    }

    template<typename T>
    inline T *get_pointer(const std::unique_ptr<T> &ptr) {
        return ptr.get();
    }

    // Adapted from google-protobuf stubs/common.h
    // see License in muduo/base/Types.h
    template<typename To, typename From>
    inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From> &f) {
        if (false) {
            implicit_cast<From *, To *>(0);
        }

#ifndef NDEBUG
        assert(f == NULL || dynamic_cast<To *>(get_pointer(f)) != NULL);
#endif
        return ::std::static_pointer_cast<To>(f);
    }

    namespace net {

        // All client visible callbacks go here.

        class Buffer;

        class TcpConnection;

        typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
        typedef std::function<void()> TimerCallback;
        typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
        typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;
        typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallback;
        typedef std::function<void(const TcpConnectionPtr &, size_t)> HighWaterMarkCallback;

        // the data has been read to (buf, len)
        typedef std::function<void(const TcpConnectionPtr &,
                                   Buffer *,
                                   Timestamp)> MessageCallback;     // 收到信息后的回调

        void defaultConnectionCallback(const TcpConnectionPtr &conn);

        void defaultMessageCallback(const TcpConnectionPtr &conn,
                                    Buffer *buffer,
                                    Timestamp receiveTime);

    }  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CALLBACKS_H