cmake_minimum_required(VERSION 3.16)
project(mymuduo)

set(CMAKE_CXX_STANDER 11)

set(net_src
        Callbacks.h
        EventLoop.h             EventLoop.cpp
        TimerId.h
        Channel.h               Channel.cpp
        SocketsOps.h            SocketsOps.cpp
        Poller.h                Poller.cpp
        TimerQueue.h            TimerQueue.cpp
        Endian.h
        Timer.h                 Timer.cpp
        )

add_library(net ${net_src})

