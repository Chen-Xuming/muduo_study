cmake_minimum_required(VERSION 3.16)
project(mymuduo)

set(CMAKE_CXX_STANDER 11)

set(net_src
        Callbacks.h
        EventLoop.h             EventLoop.cpp
        Channel.h               Channel.cpp
        SocketsOps.h            SocketsOps.cpp
        TimerQueue.h            TimerQueue.cpp
        Endian.h
        Timer.h                 Timer.cpp
        TimerId.h
        InetAddress.h           InetAddress.cpp
        Socket.h                Socket.cpp
        Buffer.h                Buffer.cpp
        Acceptor.h              Acceptor.cpp
        Connector.h             Connector.cpp
        TcpConnection.h         TcpConnection.cpp
        TcpServer.h             TcpServer.cpp
        TcpClient.h             TcpClient.cpp
        EventLoopThread.h       EventLoopThread.cpp
        EventLoopThreadPool.h   EventLoopThreadPool.cpp)

add_subdirectory(poller)
add_library(net ${net_src})
