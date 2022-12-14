//
// Created by chen on 2022/10/23.
//

#include "Logging.h"
#include "CurrentThread.h"
#include "Timestamp.h"
#include "TimeZone.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace muduo{
    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;       // 缓存最后一次进行格式化的time_t

    const char *strerror_tl(int savedErrno){
        return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
    }

    Logger::LogLevel initLogLevel(){
        if(::getenv("MUDUO_LOG_TRACE")){
            return Logger::LogLevel::TRACE;
        }else if(::getenv("MUDUO_LOG_DEBUG")){
            return Logger::DEBUG;
        }else{
            return Logger::LogLevel::INFO;
        }
    }

    Logger::LogLevel g_logLevel = initLogLevel();   // initialize global loglevel

    const char *LogLevelName[Logger::LogLevel::NUM_LOG_LEVELS] = {
            "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };

    /*
     *      用于在编译期知道string长度的辅助类
     */
    class T{
    public:
        T(const char *str, unsigned len): str_(str), len_(len){
            assert(strlen(str) == len_);
        }
        const char *str_;
        const unsigned len_;
    };

    /*
     *      把string写入LogStream
     */
    inline LogStream& operator<<(LogStream &s, T v){
        s.append(v.str_, v.len_);
        return s;
    }

    /*
     *      把文件名写入LogStream
     */
    inline LogStream& operator<<(LogStream &s, const Logger::SourceFile &v){
        s.append(v.data_, v.size_);
        return s;
    }

    /*
     *      默认的output和flush行为，都是针对stdout
     */
    void defaultOutput(const char *msg, int len){
        size_t n = fwrite(msg, 1, len, stdout);
        (void)n;    // muduo很多地方用了这种语句，目的是避免编译时的warning：“该变量未使用”
    }
    void defaultFlush(){
        fflush(stdout);
    }

    Logger::OutputFunc g_output = defaultOutput;
    Logger::FlushFunc g_flush = defaultFlush;

    TimeZone g_logTimeZone;

}   // muduo

using namespace muduo;


/*
 *      Impl构造函数
 *      在构造阶段完成写入：日期、时间、线程id、日志级别，如果存在error，也要写入
 */
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile &file, int line): time_(Timestamp::now()),
                                                                                     stream_(),
                                                                                     level_(level),
                                                                                     line_(line),
                                                                                     basename_(file){
    formatTime();
    CurrentThread::tid();
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
    stream_ << T(LogLevelName[level], 6);
    if(savedErrno != 0){
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

/*
 *      格式化时间
 *
 */
void Logger::Impl::formatTime() {
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);

    if(seconds != t_lastSecond){
        t_lastSecond = seconds;
        struct tm tm_time;
        if(g_logTimeZone.valid()){
            tm_time = g_logTimeZone.toLocalTime(seconds);
        }else{
            ::gmtime_r(&seconds, &tm_time);
        }

        int len = snprintf(t_time, sizeof (t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void)len;
    }

    if(g_logTimeZone.valid()){
        Fmt us(".%06d", microseconds);
        assert(us.length() == 8);
        stream_ << T(t_time, 17) << T(us.data(), 8);
    }else{
        Fmt us(".%06dZ ", microseconds);
        assert(us.length() == 9);
        stream_ << T(t_time, 17) << T(us.data(), 9);
    }
}

/*
 *  日志记录结束时，写入文件名和行号
 *
 */
void Logger::Impl::finish() {
    stream_ << " - " << basename_ << ":" << line_ << "\n";
}

Logger::Logger(SourceFile file, int line): impl_(INFO, 0, file, line) {

}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func) : impl_(level, 0, file, line){
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level) : impl_(level, 0, file, line){

}

Logger::Logger(SourceFile file, int line, bool toAbort): impl_(toAbort ? FATAL : ERROR, errno, file, line){

}

/*
 *  析构时调用 output()把日志写到目标位置（的缓冲区）
 */
Logger::~Logger() noexcept {
    impl_.finish();
    const LogStream::Buffer &buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if(impl_.level_ == FATAL){
        g_flush();
        abort();
    }
}


void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out) {
    g_output = out;
}

void Logger::setFlush(FlushFunc flush) {
    g_flush = flush;
}

void Logger::setTimeZone(const TimeZone &tz) {
    g_logTimeZone = tz;
}














