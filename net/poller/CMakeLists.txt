cmake_minimum_required(VERSION 3.16)
project(mymuduo)

set(CMAKE_CXX_STANDARD 11)

set(poller_src
        Poller.h        Poller.cpp
        PollPoller.h    PollPoller.cpp
        EPollPoller.h   EPollPoller.cpp
        DefaultPoller.cpp)

add_library(poller ${poller_src})
