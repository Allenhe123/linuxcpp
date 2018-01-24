#include <functional>
#include <iostream>

typedef std::function<int (int, int)> task;


int func(int a, int b)
{
	return a + b;
}


class Functor
{
	public:
		int operator() (int a, int b)
		{
			return a + b;
		}
};

int main()
{
	task t1 = func;
	std::cout<<t1(10, 9)<<std::endl;

	Functor ftor;
	task t2 = ftor;
	std::cout<<t2(8, 10)<<std::endl;


	task t3 = [](int a, int b) {return a + b;};
	std::cout<<t3(7, 10)<<std::endl;

	return 0;
}
