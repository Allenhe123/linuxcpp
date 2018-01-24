#include <iostream>
#include <functional>

int func(int a, int b)
{
    return a + b;
}

class foo
{
public:
    int func(int a, int b)
    {
        return a + b;
    }
};


int main()
{
    auto bf1 = std::bind(func, 100, std::placeholders::_1);
    std::cout<<bf1(99)<<std::endl;

    foo f;
    auto bf2 = std::bind(&foo::func, f, std::placeholders::_1, std::placeholders::_2);
    std::cout<<bf2(200, 111)<<std::endl;

    //auto bf3 = std::bind(&foo::func, f, std::placeholders::_1, 88);
    std::function<int (int)> bf3 = std::bind(&foo::func, f, std::placeholders::_1, 88);
    std::cout<<bf3(600)<<std::endl;

    return 0;
}