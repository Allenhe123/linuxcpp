#include <iostream>

template <typename T>
void foo(const T& t)
{
	std::cout<<"value: "<<t<<std::endl;
}

template <typename T>
void bar(const T& t)
{
	std::cout<<"value: "<<t<<std::endl;
}

