#include <iostream>
#include <functional>
using namespace std;

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
	auto bf1 = std::bind(func, 10, std::placeholders::_1);
	cout<<bf1(20)<<endl;

	foo f;
	auto bf2 = std::bind(&foo::func, f, std::placeholders::_1, std::placeholders::_2);
	cout<<bf2(100, 50)<<endl;

	std::function<int (int)> bf3 = std::bind(&foo::func, f, std::placeholders::_1, 100);
	cout<<bf3(100)<<endl;

	return 0;
}