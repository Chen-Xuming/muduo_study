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

namespace muduo{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    /*
     *   获取shared_ptr所指对象的指针
     */
    template<class T>
    inline T *get_pointer(const std::shared_ptr<T> &ptr){
        return ptr.get();
    }

    /*
     *  获取unique_ptr所指对象的指针
     */
    template<class T>
    inline T *get_pointer(const std::unique_ptr<T> &ptr){
        return ptr.get();
    }

    /*
     *   智能指针向上转换（static_pointer_cast是static_cast的智能指针版本）
     *   使用implicit_cast禁用向下转换
     */
    template<class To, class From>
    inline std::shared_ptr<To> down_pointer_cast(const std::shared_ptr<From> &f){
        if(false){  // 仅编译期检查
            implicit_cast<From*, To*>(0);
        }
        return std::static_pointer_cast<To>(f);
    }

    /// ------------ 一些回调 ----------------
    namespace net{
        class Buffer;
        class TcpConnection;

        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using TimerCallback = std::function<void()>;
        using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
        using CloseCallback = std::function<void(TcpConnectionPtr&)>;
        using WriteCompleteCallback = std::function<void(TcpConnectionPtr&)>;
        using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

        // 数据已经读到buffer中后的回调
        using MessageCallback = std::function<void(const TcpConnectionPtr&,
                                                   Buffer*,
                                                   Timestamp)>;

        void defaultConnectionCallback(const TcpConnectionPtr &conn);
        void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime);
    }
}

#endif  // MUDUO_NET_CALLBACKS_H
















