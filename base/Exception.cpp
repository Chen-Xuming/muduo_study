//
// Created by chen on 2022/10/20.
//

#include "Exception.h"
#include "CurrentThread.h"

namespace muduo{
    Exception::Exception(string what) : message_(std::move(what)),
                                        stack_(CurrentThread::stackTrace(false)){}
}