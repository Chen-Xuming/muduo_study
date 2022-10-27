//
// Created by chen on 2022/10/21.
//

#include "../Exception.h"
#include "../CurrentThread.h"

void function0();
void function1();
void function2();
void function3();


void function0(){
    function1();
}

void function1(){
    function2();

    std::string info = muduo::CurrentThread::stackTrace(true);
    printf("-------- stacktrace-in-function1() -------\n");
    printf("%s\n", info.c_str());
}

void function2(){
    try{
        function3();
    }catch (muduo::Exception &e){
        printf("-------- stacktrace-catch-function3-exception -------\n");
        printf("exception: %s\n", e.what());
        printf("stacktrace-info: \n%s\n", e.stackTrace());
    }
}

void function3(){
    throw muduo::Exception("exception-function3");
}

int main(){
    function0();

    return 0;
}
