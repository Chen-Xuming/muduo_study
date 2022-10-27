//
// Created by chen on 2022/10/22.
//

#include "../Atomic.h"
#include "assert.h"

int main(){
    muduo::AtomicInt32 a_int32;
    assert(a_int32.get() == 0);
    assert(a_int32.getAndAdd(100) == 0);
    assert(a_int32.get() == 100);
    assert(a_int32.addAndGet(-50) == 50);
    assert(a_int32.incrementAndGet() == 51);
    assert(a_int32.decrementAndGet() == 50);
    a_int32.add(20);
    assert(a_int32.get() == 70);
    a_int32.increment();
    assert(a_int32.get() == 71);
    a_int32.decrement();
    assert(a_int32.get() == 70);
    assert(a_int32.getAndSet(1111) == 70);
    assert(a_int32.get() == 1111);

    return 0;
}