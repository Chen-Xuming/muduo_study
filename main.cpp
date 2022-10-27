#include <iostream>

template<class T>
struct has_no_destroy{
    template<class C> static char test(decltype(&C::no_destroy));

    template<class C> static int32_t test(...);

    const static bool value = sizeof (test<T>(0)) == 1;
};

class A{
public:
    A() = default;
    ~A() = default;

    int no_destroy(){
        return 100;
    }
};



int main() {
    if(!has_no_destroy<A>::value){
        std::cout << "true" << std::endl;
    }
    else std::cout << "false" << std::endl;
    return 0;
}
