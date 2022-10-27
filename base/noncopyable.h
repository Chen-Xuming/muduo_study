//
// Created by chen on 2022/10/20.
//

#ifndef MYMUDUO_NONCOPYABLE_H
#define MYMUDUO_NONCOPYABLE_H

// 禁用拷贝构造和赋值操作
namespace muduo {

    class noncopyable {
    public:
        noncopyable(const noncopyable &) = delete;

        void operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;

        ~noncopyable() = default;
    };

}

#endif //MYMUDUO_NONCOPYABLE_H
