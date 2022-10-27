//
// Created by chen on 2022/10/22.
//

#ifndef MYMUDUO_LOGSTREAM_H
#define MYMUDUO_LOGSTREAM_H

#include "noncopyable.h"
#include "StringPiece.h"
#include "Types.h"
#include <assert.h>
#include <string.h>


namespace muduo{
    namespace detail{

        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000 * 1000;

        /*
         *      定长日志缓冲区
         *
         *      实现了一些常规操作，望文生义即可。
         *
         *      这里解释一下变量 cookie_ 的作用：
         *      muduo的日志是3秒写入磁盘一次，如果在写入磁盘之前coredump，那么这部分每写入的日志该如何找回？
         *      类中定义了函数指针 cookie_， 和data_成员相差8字节。
         *      如果发生coredump，那么通过gdb找到cookie_的位置，就能找到data_的内容：
         *
         *      gdb p &cookie：
         *      1. 如果提示它指向函数 FixedBuffer::cookieStart，那么说明data_是有内容未写入缓冲区的，
         *      2. 如果提示它指向函数 FixedBuffer::cookieEnd, 那么说明Buffer已经析构了，不存在遗失的日志。
         *
         *      参考：
         *      1. 书本P111
         *      2. https://blog.csdn.net/qq_37500516/article/details/124218503
         */
        template<int SIZE>
        class FixedBuffer: noncopyable{
        public:
            FixedBuffer(): cur_(data_){
                setCookie(cookieStart);
            }

            ~FixedBuffer(){
                setCookie(cookieEnd);
            }

            void setCookie(void(*cookie)()){
                cookie_ = cookie;
            }

            const char *data() const{
                return data_;
            }

            // 已写入数据的长度
            int length() const{
                return static_cast<int>(cur_ - data_);
            }

            // 剩余空间长度
            int avail() const{
                return static_cast<int>(end() - cur_);
            }

            char *current(){
                return cur_;
            }

            // 只有空间足够时才写入
            void append(const char *buf, size_t len){
                if(implicit_cast<size_t>(avail()) > len){
                    memcpy(cur_, buf, len);
                    cur_ += len;
                }
            }

            void add(size_t len){
                cur_ += len;
            }

            void reset(){
                cur_ = data_;
            }

            void bzero(){
                memZero(data_, sizeof data_);
            }

            // for used by GDB
            const char *debugString();

            string toString() const {
                return string(data_, length());
            }

            StringPiece toStringPiece() const{
                return StringPiece(data_, length());
            }

        private:
            const char *end() const{
                return data_ + sizeof data_;
            }

            // 空函数，因为cookie_仅是当作哨兵使用
            static void cookieStart();
            static void cookieEnd();

        private:
            void (*cookie_)();  // ”哨兵“ P111 注脚17
            char data_[SIZE];
            char *cur_;         // 写指针当前位置
        };

    }   // detail


    /*
     *      定义了Log的输出流
     *
     *      具体而言，定义各种 operator<<(T)，即给定某种类型的数据，将其格式化存储到 FixedBuffer里面
     *
     */
    class LogStream: noncopyable{
        using self = LogStream;

    public:
        using Buffer = detail::FixedBuffer<detail::kSmallBuffer>;

        self& operator<<(bool);
        self& operator<<(short);
        self& operator<<(unsigned short);
        self& operator<<(int);
        self& operator<<(unsigned int);
        self& operator<<(long);
        self& operator<<(unsigned long);
        self& operator<<(long long);
        self& operator<<(unsigned long long);
        self& operator<<(const void*);
        self& operator<<(float);
        self& operator<<(double);
        self& operator<<(char);
        self& operator<<(const char*);
        self& operator<<(const unsigned char*);
        self& operator<<(const string&);
        self& operator<<(const StringPiece&);
        self& operator<<(const Buffer&);

        void append(const char *data, int len);

        const Buffer& buffer() const;
        void resetBuffer();

    private:
        void staticCheck();

        template<class T>
        void formatInteger(T);

    private:
        Buffer buffer_;
        static const int kMaxNumericSize = 48;  // 最大数值类型占用的空间大小
    };  // class LogStream

    /*
     *      单个数据的格式化类
     */
    class Fmt{
    private:
        char buf_[32];
        int length_;

    public:
        /*
         *      将数据val按照格式fmt存储到buf_里面.
         */
        template<class T>
        Fmt(const char *fmt, T val){
            // 检查T是否为数值类型
            static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

            length_ = snprintf(buf_, sizeof buf_, fmt, val);
            assert(static_cast<size_t>(length_) < sizeof buf_);
        }

        const char *data() const{
            return buf_;
        }

        int length() const {
            return length_;
        }
    };

    /*
     *      将一个数据输出到LogStream的流程
     *      1. 即将数据格式化，存到Fmt.buf_
     *      2. 将Fmt.buf_追加到LogStream中
     */
    inline LogStream& operator<<(LogStream &s, const Fmt &fmt){
        s.append(fmt.data(), fmt.length());
        return s;
    }


    string formatSI(int64_t n);
    string formatIEC(int64_t n);
}

#endif //MYMUDUO_LOGSTREAM_H













