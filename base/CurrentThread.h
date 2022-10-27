//
// Created by chen on 2022/10/20.
//

#ifndef MYMUDUO_CURRENTTHREAD_H
#define MYMUDUO_CURRENTTHREAD_H

#include "Types.h"

namespace muduo{
    namespace CurrentThread {
        extern __thread int t_cachedTid;        // 缓存当前线程的tid
        extern __thread char t_tidString[32];   // tid字符串
        extern __thread int t_tidStringLength;  // tid字符串长度
        extern __thread const char *t_threadName;   // 线程名

        void cacheTid();        // 记录当前线程的tid
        bool isMainThread();
        string stackTrace(bool demangle);   // 跟踪调用栈
        void sleepUsec(int64_t usec);

        /*
         *  __builtin_expect用于编译优化
         *
         *  if(__builtin_expect(exp, N){
         *      foo();
         *  }else{
         *      ...
         *  }
         *
         *  __builtin_expect(exp, 0) 表示期望 exp==0, 执行foo的可能性很小
         *  __builtin_expect(exp, 1) 表示期望 exp==1 成立，执行foo的可能性很大
         *
         *  如果抛开性能，实际上等价于 if(exp)，跟N无关
         *
         */
        inline int tid(){
            if(__builtin_expect(t_cachedTid == 0, 0)){  // t_cachedTid == 0 可能性很小
                cacheTid();
            }
            return t_cachedTid;
        }

        // 打日志用
        inline const char *tidString(){
            return t_tidString;
        }

        // 打日志用
        inline int tidStringLength(){
            return t_tidStringLength;
        }

        inline const char *name(){
            return t_threadName;
        }
    }
}


#endif //MYMUDUO_CURRENTTHREAD_H
