//
// Created by chen on 2022/10/21.
//

/*
 *      定义整形原子类型
 *      实际上可以直接使用 std::Atomic<>
 *
 */

#ifndef MYMUDUO_ACTOMIC_H
#define MYMUDUO_ACTOMIC_H

#include "noncopyable.h"
#include <stdint.h>

namespace muduo{
    namespace detail{

        template<typename T>
        class AtomicIntegerT: noncopyable{
        public:
            AtomicIntegerT(): value_(0){}

            /*
             *      获取当前值。原子操作。
             *      相当于atomic.load();
             */
            T get(){
                return __atomic_load_n(&value_, __ATOMIC_SEQ_CST);
            }

            /*
             *      value_ += x
             *      但返回旧值。
             *      是原子操作
             */
            T getAndAdd(T x){
                return __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST);
            }

            /*
             *      自增，且返回新值
             *
             *      该函数是原子操作吗？
             *      “getAndAdd(x) + x”不是原子操作，但其达到了原子操作的目的。
             *      参看 questions.txt question1
             */
            T addAndGet(T x){
                return getAndAdd(x) + x;
            }

            /*
             *      自增1，且返回新值
             *      相当于 ++value_;
             */
            T incrementAndGet(){
                return addAndGet(1);
            }

            /*
             *      自减1，且返回新值
             *      相当于 --value_;
             */
            T decrementAndGet(){
                return addAndGet(-1);
            }

            /*
             *      自增x，不返回
             */
            void add(T x){
                getAndAdd(x);
            }

            void increment(){
                incrementAndGet();
            }

            void decrement(){
                decrementAndGet();
            }

            /*
             *      value_ = newValue
             *      但返回旧值。
             *      原子操作
             */
            T getAndSet(T newValue){
                return __atomic_exchange_n(&value_, newValue, __ATOMIC_SEQ_CST);
            }

        private:
            volatile T value_;  // volatile：确保本条指令不会被编译器优化。且每次读value_值时都要去访问内存，而不是读它的缓存值
        };
    }

    using AtomicInt32 = detail::AtomicIntegerT<int32_t>;
    using AtomicInt64 = detail::AtomicIntegerT<int64_t>;
}


#endif //MYMUDUO_ACTOMIC_H
