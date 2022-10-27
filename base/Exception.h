//
// Created by chen on 2022/10/20.
//

// 定义异常类，实际上和std::exception差不多

#ifndef MYMUDUO_EXCEPTION_H
#define MYMUDUO_EXCEPTION_H

#include "Types.h"
#include <exception>

namespace muduo{
    class Exception: public std::exception{
    public:
        Exception(string what);
        ~Exception() noexcept override = default;

        const char *what() const noexcept override{
            return message_.c_str();
        }

        const char *stackTrace() const noexcept{
            return stack_.c_str();
        }

    private:
        string message_;
        string stack_;
    };
}




#endif //MYMUDUO_EXCEPTION_H
