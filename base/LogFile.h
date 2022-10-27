//
// Created by chen on 2022/10/25.
//

/*
 *      LogFile
 *
 *      1. 日志滚动（rolling）的两个条件：
 *          a. 文件大小达到上限（1GB）
 *          b. 定期滚动（每日零点新建日志文件）
 *
 *      2. 日志文件的命名方式
 *      [进程名].[创建时间].[主机名].[进程id].log
 *      logfile_test.20120603-144022.hostname.3605.log
 *      注意：日志文件名中的时间是创建时间，并不代表其中所有日志都是当前写入的，见下面第4条
 *
 *      3. 写入磁盘（flush）的条件：缓冲中写够N条日志，且两次flush的间隔要求大于3秒
 *
 *      4. “定时滚动”不一定是每天0点时立即创建一个新的日志文件，
 *         根据 append_unlocked() 的逻辑，只有在写够 checkEveryN 条日志时，
 *         才检查是否跨了一天，从而进行滚动
 *         /// FIXME 如果6月3日每写满checkEveryN，6月4日也未写满checkEveryN，知道6月5日才写满，
 *         /// FIXME 那么6月4日的日志会写在6月3日创建的日志文件里面
 */


#ifndef MYMUDUO_LOGFILE_H
#define MYMUDUO_LOGFILE_H

#include "Mutex.h"
#include "Types.h"

#include <memory>

namespace muduo {

    namespace FileUtil {
        class AppendFile;
    }

    class LogFile : noncopyable {
    public:
        LogFile(const string &basename,
                off_t rollSize,
                bool threadSafe = true,
                int flushInterval = 3,
                int checkEveryN = 1024);

        ~LogFile();

        void append(const char *logline, int len);

        void flush();

        bool rollFile();

    private:
        void append_unlocked(const char *logline, int len);

        static string getLogFileName(const string &basename, time_t *now);

        const string basename_;     // 程序名
        const off_t rollSize_;      // 单个文件写满rollSize_大小时自动滚动

        // 当写够checkEveryN_条日志，且两次flush时间间隔大于flushInterval_时，
        // 写入（flush）磁盘一次
        const int flushInterval_;
        const int checkEveryN_;
        int count_;

        std::unique_ptr<MutexLock> mutex_;
        time_t startOfPeriod_;  // 当天0点的时间
        time_t lastRoll_;
        time_t lastFlush_;
        std::unique_ptr<FileUtil::AppendFile> file_;

        const static int kRollPerSeconds_ = 60 * 60 * 24;
    };

}  // namespace muduo

#endif //MYMUDUO_LOGFILE_H
