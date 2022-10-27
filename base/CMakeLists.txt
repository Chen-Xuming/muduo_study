cmake_minimum_required(VERSION 3.16)
project(mymuduo)

set(CMAKE_CXX_STANDARD 11)


set(base_src
        Types.h
        copyable.h
        noncopyable.h
        Date.h                  Date.cpp
        Timestamp.h             TimeStamp.cpp
        TimeZone.h              TimeZone.cpp
        Singleton.h
        Exception.h             Exception.cpp
        CurrentThread.h         CurrentThread.cpp
        Mutex.h
        Condition.h             Condition.cpp
        CountDownLatch.h        CountDownLatch.cpp
        Atomic.h
        WeakCallback.h
        StringPiece.h
        LogStream.h             LogStream.cpp
        Logging.h               Logging.cpp
        FileUtil.h              FileUtil.cpp
        ProcessInfo.h           ProcessInfo.cpp
        LogFile.h               LogFile.cpp
        BlockingQueue.h
        BoundedBlockingQueue.h
        AsyncLogging.h          AsyncLogging.cpp
        Thread.h                Thread.cpp
        ThreadLocal.h
        ThreadLocalSingleton.h
        ThreadPool.h            ThreadPool.cpp
        )

add_library(base ${base_src})

add_subdirectory(testcase)
target_link_libraries(base pthread)

#install(TARGETS muduo_base DESTINATION lib)
#
#file(GLOB HEADERS "*.h")
#install(FILES ${HEADERS} DESTINATION include/muduo/base)
#
#if (MUDUO_BUILD_EXAMPLES)
#    add_subdirectory(tests)
#endif ()