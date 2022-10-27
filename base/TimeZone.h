//
// Created by chen on 2022/10/20.
//

// 定义时区类(1970-2030)

#ifndef MYMUDUO_TIMEZONE_H
#define MYMUDUO_TIMEZONE_H

#include "copyable.h"
#include <memory>       // for shared_ptr
#include <time.h>

namespace muduo{
    class TimeZone : public muduo::copyable{

    public:
        struct Data;

        explicit TimeZone(const char *zonefile);    // 根据zonefile初始化时区

        TimeZone(int east0fUtc, const char *tzname);    // 固定时区，即东经0度的时区
        TimeZone() = default;

        bool valid() const {
            return static_cast<bool>(data_);
        }

        struct tm toLocalTime(time_t secondsSinceEpoch) const;      // secondsSinceEpoch秒数 ==> tm格式本地时间(UTC+N)
        time_t fromLocalTime(const struct tm&) const;       // tm格式本地时间 ==> secondsSinceEpoch秒数

        static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false); // 将secondsSinceEpoch秒数转化为格林尼治时间（UTC+0）
        static time_t fromUtcTime(const struct tm &);       // tm格林尼治时间 ==> secondsSinceEpoch秒数

        // year in [1900..2500], month in [1..12], day in [1..31]
        // ymd-hms ==> secondsSinceEpoch秒数
        static time_t fromUtcTime(int year, int month, int day,
                                  int hour, int minute, int seconds);

    private:
        std::shared_ptr<Data> data_;
    };
}


#endif //MYMUDUO_TIMEZONE_H
