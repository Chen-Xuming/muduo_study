//
// Created by chen on 2022/10/27.
//

/*
 *      线程单例
 *
 */

#ifndef MYMUDUO_THREADLOCALSINGLETON_H
#define MYMUDUO_THREADLOCALSINGLETON_H

#include "noncopyable.h"

#include <assert.h>
#include <pthread.h>

namespace muduo {

    template<typename T>
    class ThreadLocalSingleton : noncopyable {
    public:
        ThreadLocalSingleton() = delete;

        ~ThreadLocalSingleton() = delete;

        static T &instance() {
            if (!t_value_) {
                t_value_ = new T();
                deleter_.set(t_value_);
            }
            return *t_value_;
        }

        static T *pointer() {
            return t_value_;
        }

    private:
        static void destructor(void *obj) {
            assert(obj == t_value_);
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void) dummy;
            delete t_value_;
            t_value_ = 0;
        }

        /*
         *      ~ThreadLocalSingleton()被禁用了，所以用Deleter来释放t_value
         */
        class Deleter {
        public:
            Deleter() {
                pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
            }

            ~Deleter() {
                pthread_key_delete(pkey_);
            }

            void set(T *newObj) {
                assert(pthread_getspecific(pkey_) == nullptr);
                pthread_setspecific(pkey_, newObj);
            }

            pthread_key_t pkey_;
        };

        static __thread T *t_value_;        // Singleton Object
        static Deleter deleter_;
    };

    template<typename T>
    __thread T *ThreadLocalSingleton<T>::t_value_ = 0;

    template<typename T>
    typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}  // namespace muduo

#endif //MYMUDUO_THREADLOCALSINGLETON_H
