cmake_minimum_required(VERSION 3.16)
project(mymuduo)

set(CMAKE_CXX_STANDARD 11)
SET (CMAKE_ENABLE_EXPORTS TRUE)     # For stacktrace_test. 具体而言，是让::backtrace_symbols函数能够在堆栈信息中包含函数名

set(CXX_FLAGS
        -g
        -std=c++11
        -rdynamic
        )

link_libraries(pthread)

add_executable(date_test Date_test.cpp)
target_link_libraries(date_test base)

add_executable(timestamp_test Timestamp_test.cpp)
target_link_libraries(timestamp_test base)

add_executable(timezone_test TimeZone_test.cpp)
target_link_libraries(timezone_test base)

add_executable(stacktrace_test stacktrace_test.cpp)
target_link_libraries(stacktrace_test base)
add_executable(stacktrace_test2 stacktrace_test2.cpp)
target_link_libraries(stacktrace_test2 base)

add_executable(atomic_test Atomic_test.cpp)
target_link_libraries(atomic_test base)

add_executable(threadLocal_test ThreadLocal_test.cpp)
target_link_libraries(threadLocal_test base)

add_executable(threadlocalSingleton_test ThreadLocalSingleton_test.cpp)
target_link_libraries(threadlocalSingleton_test base)