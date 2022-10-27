//
// Created by chen on 2022/10/23.
//

/*
 *      日志类Logger
 *
 *      1. Logger不直接负责写日志操作，而是设置日志属性（日志等级，时区等）
 *      2. 写日志的工作由内部类Impl代劳：Imlp调用logStream::operator<<()，而更底层的细节（对FixedBuffer的操作）由LogStream实现
 *      3. Impl写完之后，Logger调用g_output()将LogStream的内容输出到指定位置（缓冲区），默认是stdout
 *      4. 如果日志等级是FATAL，则还要手动调用g_flush()立即输出到目标位置。否则目标缓冲区满的时候会自行flush
 *
 *      每条日志格式如下：
 *      日期 时间.微秒 线程 级别 正文 - 源文件名：行号
 *      20120603 08:02:46.125770Z 23261 INFO Hello - test.cpp:51
 *
 */


#ifndef MYMUDUO_LOGGING_H
#define MYMUDUO_LOGGING_H

#include "LogStream.h"
#include "Timestamp.h"
#include "TimeZone.h"

namespace muduo{
    class Logger{
    public:
        enum LogLevel{
            TRACE,
            DEBUG,
            INFO,           // default
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,     // 6种级别
        };

        /*
         *      Sourcefile 用来解析和保存源文件名及其长度
         *
         */
        class SourceFile{
        public:
            const char *data_;
            int size_;

            template<int N>
            SourceFile(const char(&arr)[N]): data_(arr), size_(N-1){
                // 一个路径最后一个"/"之后即为文件名
                const char *slash = strrchr(data_, '/');
                if(slash){
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }

            explicit SourceFile(const char *filename): data_(filename){
                const char *slash = strrchr(data_, '/');
                if(slash){
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }
        };  // class SourceFile


        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);    // func是函数名
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        LogStream &stream() {
            return impl_.stream_;
        }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);

        // 用户可以自定义output和flush
        // 默认是output到stdout，fflush(stdout)
        typedef void (*OutputFunc)(const char *msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);

        static void setTimeZone(const TimeZone &tz);

    private:
        /*
         *      Impl类负责输出日志（通过操作LogStream类）
         *
         */
        class Impl{
        public:
            using LogLevel = Logger::LogLevel;
            Impl(LogLevel level, int old_error, const SourceFile &file, int line);

            void formatTime();
            void finish();

            Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
            SourceFile basename_;
        };

        Impl impl_;
    };

    extern Logger::LogLevel g_loglevel;     // global loglevel
    inline Logger::LogLevel Logger::logLevel() {
        return g_loglevel;
    }

/*
 *      定义一些 LOG_* 宏方便使用
 *      1. 用法： LOG_* << "..." << "......";
 *      2. 附带过滤功能，如果LOG_*比当前的loglevel低级，那么这些Log语句是空操作，不影响性能。
 *      3. WARN / ERROR / FATAL 这些级别的日志是很重要的，所以不能过滤
 *
 */
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

    /*
     *      根据errno返回相应的错误提示
     */
    const char *strerror_tl(int savedError);


// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.
//
// 检查指针是否为nullptr

#define CHECK_NOTNULL(val) \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

    // A small helper for CHECK_NOTNULL().
    template<typename T>
    T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr) {
        if (ptr == NULL) {
            Logger(file, line, Logger::FATAL).stream() << names;
        }
        return ptr;
    }
}



#endif //MYMUDUO_LOGGING_H















