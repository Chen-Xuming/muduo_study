//
// Created by chen on 2022/10/21.
//

#include "../Exception.h"

void function0();
void function1();
void function2();
void function3();


void function0(){
    function1();
}

void function1(){
    function2();
}

void function2(){
    try{
        function3();
    }catch (muduo::Exception &e){
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
