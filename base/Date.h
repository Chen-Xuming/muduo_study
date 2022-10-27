//
// Created by chen on 2022/10/19.
//

// 日期类（Gregorian calendar）

#include "copyable.h"
#include "Types.h"

struct tm;

/// Gregorian calendar
// 注：JulianDay 表示 距离公元前4713年1月1日的天数
namespace muduo{
    class Date: public muduo::copyable{
    public:
        struct YearMonthDay{
            int year;
            int month;  // [1-12]
            int day;    // [1-31]
        };

        static const int kDaysPerWeek = 7;
        static const int kJulianDayOf1970_01_01;    // 19700101的儒略日

        Date():julianDayNumber_(0){}
        Date(int year, int month, int day);   // yyyy-mm-dd
        explicit Date(int julianDayNum): julianDayNumber_(julianDayNum){}
        explicit Date(const struct tm &);

        void swap(Date &that) {
            std::swap(julianDayNumber_, that.julianDayNumber_);
        }

        bool valid() const {
            return julianDayNumber_ > 0;
        }

        // julianday to yyyy-mm-dd format
        string toIsoString() const;

        struct YearMonthDay yearMonthDay() const;

        int year() const {
            return yearMonthDay().year;
        }

        int month() const {
            return yearMonthDay().month;
        }

        int day() const {
            return yearMonthDay().day;
        }

        // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
        int weekDay() const {
            return (julianDayNumber_ + 1) % kDaysPerWeek;
        }

        int julianDayNumber() const {
            return julianDayNumber_;
        }

    private:
        int julianDayNumber_;
    };


    inline bool operator<(Date x, Date y) {
        return x.julianDayNumber() < y.julianDayNumber();
    }

    inline bool operator==(Date x, Date y) {
        return x.julianDayNumber() == y.julianDayNumber();
    }
}