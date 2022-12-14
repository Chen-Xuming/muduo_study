//
// Created by chen on 2022/10/22.
//

/*
 *      Google StringPiece Class
 *      Author: Sanjay Ghemawat
 *
 *      StringPiece不是字符串变量本身（std::string, char *, const char *等），而仅是用一个指针指向它们所在的内存位置而已。
 *      StringPiece只有两个成员变量：
 *          const char *ptr_;
 *          int length_;
 *      ptr_可以指向字符串任意一个地方，length_表示”piece“的长度
 *
 *      设计StringPiece的目的：
 *      1. 避免字符串拷贝(只读场景)。
 *          例如，函数参数为std::string，当传入const char*时，会生成一个string对象，发生了拷贝。
 *          而StringPiece仅记录起始地址和长度，无拷贝。当然，指针失效是有可能的。
 *      2. StringPiece构造函数接受多种字符串类型（const char*, const string&等， 这些const也说明StringPiece仅用于只读的场景）。
 *          类型的统一使函数的编写很方便，去冗余
 *
 *      StringPiece还定义了许多操作，参见代码实现
 */


#ifndef MYMUDUO_STRINGPIECE_H
#define MYMUDUO_STRINGPIECE_H

#include <string.h>
#include <iosfwd>    // for ostream forward-declaration

#include "Types.h"

namespace muduo {

// For passing C-style string argument to a function.
    class StringArg // copyable
    {
    public:
        StringArg(const char *str)
                : str_(str) {}

        StringArg(const string &str)
                : str_(str.c_str()) {}

        const char *c_str() const { return str_; }

    private:
        const char *str_;
    };

    class StringPiece {
    private:
        const char *ptr_;
        int length_;

    public:
        // We provide non-explicit singleton constructors so users can pass
        // in a "const char*" or a "string" wherever a "StringPiece" is
        // expected.
        StringPiece()
                : ptr_(NULL), length_(0) {}

        StringPiece(const char *str)
                : ptr_(str), length_(static_cast<int>(strlen(ptr_))) {}

        StringPiece(const unsigned char *str)
                : ptr_(reinterpret_cast<const char *>(str)),
                  length_(static_cast<int>(strlen(ptr_))) {}

        StringPiece(const string &str)
                : ptr_(str.data()), length_(static_cast<int>(str.size())) {}

        StringPiece(const char *offset, int len)
                : ptr_(offset), length_(len) {}

        // data() may return a pointer to a buffer with embedded NULs, and the
        // returned buffer may or may not be null terminated.  Therefore it is
        // typically a mistake to pass data() to a routine that expects a NUL
        // terminated string.  Use "as_string().c_str()" if you really need to do
        // this.  Or better yet, change your routine so it does not rely on NUL
        // termination.
        const char *data() const { return ptr_; }

        int size() const { return length_; }

        bool empty() const { return length_ == 0; }

        const char *begin() const { return ptr_; }

        const char *end() const { return ptr_ + length_; }

        void clear() {
            ptr_ = NULL;
            length_ = 0;
        }

        void set(const char *buffer, int len) {
            ptr_ = buffer;
            length_ = len;
        }

        void set(const char *str) {
            ptr_ = str;
            length_ = static_cast<int>(strlen(str));
        }

        void set(const void *buffer, int len) {
            ptr_ = reinterpret_cast<const char *>(buffer);
            length_ = len;
        }

        char operator[](int i) const { return ptr_[i]; }

        void remove_prefix(int n) {
            ptr_ += n;
            length_ -= n;
        }

        void remove_suffix(int n) {
            length_ -= n;
        }

        bool operator==(const StringPiece &x) const {
            return ((length_ == x.length_) &&
                    (memcmp(ptr_, x.ptr_, length_) == 0));
        }

        bool operator!=(const StringPiece &x) const {
            return !(*this == x);
        }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                             \
  bool operator cmp (const StringPiece& x) const {                           \
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
    return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
  }

        STRINGPIECE_BINARY_PREDICATE(<, <);

        STRINGPIECE_BINARY_PREDICATE(<=, <);

        STRINGPIECE_BINARY_PREDICATE(>=, >);

        STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

        int compare(const StringPiece &x) const {
            int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
            if (r == 0) {
                if (length_ < x.length_) r = -1;
                else if (length_ > x.length_) r = +1;
            }
            return r;
        }

        string as_string() const {
            return string(data(), size());
        }

        void CopyToString(string *target) const {
            target->assign(ptr_, length_);
        }

        // Does "this" start with "x"
        bool starts_with(const StringPiece &x) const {
            return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
        }
    };

}  // namespace muduo

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<muduo::StringPiece> {
  typedef __true_type    has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __true_type    is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream &operator<<(std::ostream &o, const muduo::StringPiece &piece);

#endif //MYMUDUO_STRINGPIECE_H
