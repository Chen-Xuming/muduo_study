//
// Created by chen on 2022/10/20.
//

/// 单例模式
/// 基于 pthread_once(或者call_once, c++11) 和 noncopyable实现


#ifndef MYMUDUO_SINGLETON_H
#define MYMUDUO_SINGLETON_H

#include "noncopyable.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo{
    namespace detail{

        /*
         *    用于检测 T类 有没有定义成员函数 no_destroy
         *
         *    ---工作原理---
         *    test<T>有两份重载，当运行 sizeof(test<T>(0)) == 1 时，会去找匹配的函数
         *    a. 如果类T定义了 no_destroy 函数，那么 test(decltype(&C::no_destroy)) 匹配，返回char，继而 sizeof (test<T>(0)) == 1 成立
         *    b. 否则，test(...)肯定匹配，返回int32_t，sizeof (test<T>(0)) == 1 不成立
         *
         *    注：这里的sizeof只是查看test的返回值的大小，在编译期就能完成，且无需提供函数体
         *
         */
        template<class T>
        struct has_no_destroy{
            template<class C> static char test(decltype(&C::no_destroy));

            template<class C> static int32_t test(...);

            const static bool value = sizeof (test<T>(0)) == 1; // 0在这里有暗指nullptr的意思
        };
    }

    template<class T>
    class Singleton: noncopyable{
    public:
        Singleton() = delete;
        ~Singleton() = delete;

        static T &getInstance(){
            pthread_once(&ponce_, &Singleton::init);    // 保证init只执行一次，且完整执行
            assert(value_ != nullptr);
            return *value_;
        }

    private:
        static void init(){
            value_ = new T();
            if(!detail::has_no_destroy<T>::value){      // 如果没有定义no_destroy函数，则为其注册destroy
                ::atexit(destroy);
            }
        }

        static void destroy(){
            /*
             *  T必须是完全类型，否则编译器会报错
             */
            typedef char T_must_be_complete_type[sizeof (T) == 0 ? -1 : 1];
            T_must_be_complete_type  dummy;
            (void)dummy;

            delete value_;
            value_ = nullptr;
        }

        static pthread_once_t ponce_;
        static T *value_;
    };

    template<class T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template<class T>
    T* Singleton<T>::value_ = nullptr;
}


#endif //MYMUDUO_SINGLETON_H

