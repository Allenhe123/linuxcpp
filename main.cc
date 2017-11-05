#include "template.h"

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

int main()
{
	foo(10);
	bar(4.56);
	return 0;
}
