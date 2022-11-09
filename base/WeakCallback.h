//
// Created by chen on 2022/10/22.
//

/*
 *      弱回调:
 *      在回调时尝试将weak_ptr提升（lock）为shared_ptr，
 *      如果提升成功，则调用回调函数
 *
 */

#ifndef MYMUDUO_WEAKCALLBACK_H
#define MYMUDUO_WEAKCALLBACK_H

#include <functional>
#include <memory>

namespace muduo{
    template<class Class, class... Args>
    class WeakCallBack{
    public:
        WeakCallBack(const std::weak_ptr<Class> &object,
                     const std::function<void(Class*, Args...)> &function)
                     : object_(object), function_(function){

        }

        /*
         *      尝试回调
         */
        void operator()(Args &&... args) const {
            std::shared_ptr<Class> ptr(object_.lock()); // 尝试提升
            if(ptr){
                function_(ptr.get(), std::forward<Args>(args)...);
            }
        }

    private:
        std::weak_ptr<Class> object_;
        std::function<void(Class*, Args...)> function_;  // void(class_ptr, ...);
    };

    /*
     *   普通成员函数作为回调函数
     */
    template<class Class, class ...Args>
    WeakCallBack<Class, Args...> makeWeakCallback(const std::shared_ptr<Class> &object,
                                                  void(Class::*function)(Args...)){
        return WeakCallBack<Class, Args...>(object, function);
    }

    /*
     *   const成员函数作为回调函数
     */
    template<class CLASS, class ...Args>
    WeakCallBack<CLASS, Args...> makeWeakCallback(const std::shared_ptr<CLASS> &object,
                                                  void (CLASS::*function)(Args...) const) {
        return WeakCallBack<CLASS, Args...>(object, function);
    }
}


#endif //MYMUDUO_WEAKCALLBACK_H
