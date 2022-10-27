//
// Created by chen on 2022/10/19.
//

// implicit_cast 和 down_cast 是 google 的设计

#ifndef MUDUO_STUDY_TYPES_H
#define MUDUO_STUDY_TYPES_H

#include <stdint.h>
#include <string.h>     // memset
#include <string>

#ifndef NDEBUG
#include <assert.h>
#endif

namespace muduo{
    using std::string;

    inline void memZero(void *p, size_t n){
        memset(p, 0, n);
    }

    /*
     *  隐式转换, static_cast 和 const_cast 的安全版本。
     *  支持向上转换 / 非const转const
     *
     *  用法：implicit_cast<ToType>(expr);     // FromType自动识别
     *
     *  Q: 为什么安全？
     *  A: static_cast支持向上和向下转换，但向下转换是不安全的。
     *     implicit_cast 虽然比 static_cast弱，但是保证安全，编译器会给出提示。
     */
    template<class To, class From>
    inline To implicit_cast(From const &f){
        return f;
    }

    /*
     *  向下类型转换
     *  在debug模式，使用dynamic_cast来保证（或者说检查）转换是合法的（如果不合法，程序crash！）
     *  在release模式，使用static_cast进行转换，它比static_cast效率高，且安全（虽然static_cast向下转换本身不安全，但是debug模式已验证该转换是安全的）
     *
     *  用法：down_cast<T*>(foo)
     */
    template<class To, class From>
    inline To down_cast(From *f){

        /*
         *   确保To是From*的子类
         *   这只会在编译期间检查，因为 if(false) 会让编译器把这句优化掉
         */
        if(false){
            implicit_cast<From *, To>(0);
        }

#if !defined(NDEBUG)
        assert(f == nullptr || dynamic_cast<To>(f) != nullptr);
#endif

        return static_cast<To>(f);
    }

}

#endif //MUDUO_STUDY_TYPES_H
