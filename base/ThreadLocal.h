//
// Created by chen on 2022/10/27.
//

#ifndef MYMUDUO_THREADLOCAL_H
#define MYMUDUO_THREADLOCAL_H

/*
 *      为什么不直接使用__thread或者thread_local?
 *
 */

/*
 *      1. pthread_key_create创建一个thread local的key，不同线程访问这个key得到不同的值. 此外，还可以定义所绑定数据的析构行为
 *      2. pthread_getspecific / pthread_setspecific 用于get/set key对应的值，该值因线程而异
 *      3. pthread_key_delete删除这个key，并调用create时绑定的回调函数
 *
 */

#include "Mutex.h"
#include "noncopyable.h"
#include <pthread.h>

namespace muduo {

    template<typename T>
    class ThreadLocal : noncopyable {
    public:
        ThreadLocal() {
            pthread_key_create(&pkey_, &ThreadLocal::destructor);
        }

        ~ThreadLocal() {
            pthread_key_delete(pkey_);
        }

        /*
         *   懒汉式初始化
         */
        T &value() {
            T *perThreadValue = static_cast<T *>(pthread_getspecific(pkey_));
            if (!perThreadValue) {
                T *newObj = new T();
                pthread_setspecific(pkey_, newObj);
                perThreadValue = newObj;
            }
            return *perThreadValue;
        }

    private:

        static void destructor(void *x) {
            T *obj = static_cast<T *>(x);
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void) dummy;
            delete obj;
        }

    private:
        pthread_key_t pkey_;
    };

}  // namespace muduo


#endif //MYMUDUO_THREADLOCAL_H
