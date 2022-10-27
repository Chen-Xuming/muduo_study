//
// Created by chen on 2022/10/19.
//

#ifndef MYMUDUO_COPYABLE_H
#define MYMUDUO_COPYABLE_H

/*
 *  所有可拷贝类的基类
 *  注意：这些类应该为值语义
 */
namespace muduo{
    class copyable{
    protected:
        copyable() = default;
        ~copyable() = default;
    };
}

#endif //MYMUDUO_COPYABLE_H
