//
// Created by chen on 2022/10/20.
//

#include "CurrentThread.h"
#include "Timestamp.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <execinfo.h>
#include <cxxabi.h>


namespace muduo{

    namespace detail{
        // 获取线程id
        // 注意：pthread_self()也能获得线程id，但是不同进程的线程id可以相同，这使属于不同进程的线程通信带来麻烦
        // 而syscall(SYS_gettid)能够得到全局唯一线程id
        pid_t gettid(){
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

    // 记录当前线程tid
    void CurrentThread::cacheTid() {
        if(t_cachedTid == 0){
            t_cachedTid = detail::gettid();
            t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid); // 返回格式化字符串的长度
        }
    }

    bool CurrentThread::isMainThread() {
        return tid() == ::getpid();
    }

    // 线程睡眠若干纳秒
    void CurrentThread::sleepUsec(int64_t usec){
        struct timespec ts = {0, 0};    // 秒，纳秒
        ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
        ::nanosleep(&ts, nullptr);
    }

    namespace CurrentThread{
        __thread int t_cachedTid = 0;
        __thread char t_tidString[32];
        __thread int t_tidStringLength = 6;
        __thread const char *t_threadName = "unknown";
        static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");


        /*
         *      获取堆栈信息
         *
         *      demangle为true时，对堆栈信息进行“解构”。例如：
         *      ::backtrace_symbols() 获得的原始信息是（其中一行，这是testcase/stacktrace_test2的运行结果）：
         *          /home/bear/muduo-study/cmake-build-debug/base/testcase/stacktrace_test2(_Z9function3v+0x5d) [0x5637b77036c1]
         *          编译器会把function3重命名为_Z9function3v
         *      通过 abi::__cxa_demangle() 可以将 “_Z9function3v” 重新解析为原函数名 "function3()"
         *
         *      题外话：
         *      要使用编译选项 “SET (CMAKE_ENABLE_EXPORTS TRUE)” 才能让 ::backtrace_symbols函数 获取调用栈上的函数名。
         *      https://stackoverflow.com/questions/6934659/how-to-make-backtrace-backtrace-symbols-print-the-function-names
         */
        string stackTrace(bool demangle){

            string stack;
            const int max_frames = 200;
            void *frame[max_frames];        // 用于存储堆栈信息

            /*
             *  1. 获取堆栈信息，存储与frame数组，最多max_frames，返回的是实际条数
             *  2. 把frame里面的信息转为string字符串（数组）
             */
            int nptrs = ::backtrace(frame, max_frames);
            char **strings = ::backtrace_symbols(frame, nptrs);

            if(strings){
                size_t len = 512;
                char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr;

                for(int i = 1; i < nptrs; i++){     // 跳过第0条，即本函数
                    if(demangle){
                        char *left_par = nullptr;
                        char *plus = nullptr;
                        for(char *p = strings[i]; *p; ++p){
                            if(*p == '('){
                                left_par = p;
                            }
                            else if(*p == '+'){
                                plus = p;
                            }
                        }

                        if(left_par && plus){
                            *plus = '\0';
                            int status = 0;
                            char *ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
                            *plus = '+';
                            if(status == 0){
                                demangled = ret;
                                stack.append(strings[i], left_par + 1);
                                stack.append(demangled);
                                stack.append(plus);
                                stack.push_back('\n');
                                continue;
                            }
                        }
                    }

                    stack.append(strings[i]);
                    stack.push_back('\n');
                }
                free(demangled);
                free(strings);
            }
            return stack;
        }

    }

}
