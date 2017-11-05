
#include <iostream>
#include <functional>
#include <vector>
using namespace std;

// c type global function
int c_func(int a, int b)
{
	return a + b;
}

//function object
class functor
{
public:
	int operator() (int a, int b)
	{
		return a + b;
	}
};

int main()
{
	//old use way
	typedef int (*F)(int, int);
	F f = c_func;
	cout<<f(1,2)<<endl;

	functor ft;
	cout<<ft(1,2)<<endl;

    /////////////////////////////////////////////

	std::function< int (int, int) > myfunc;

	myfunc = c_func;
	cout<<myfunc(3,4)<<endl;

	myfunc = ft;
	cout<<myfunc(3,4)<<endl;

	//lambda
	myfunc = [](int a, int b) { return a + b;};
	cout<<myfunc(3, 4)<<endl;

	//////////////////////////////////////////////

	std::vector<std::function<int (int, int)> > v;
	v.push_back(c_func);
	v.push_back(ft);
	v.push_back([](int a, int b) { return a + b;});

	for (const auto& e : v)
	{
		cout<<e(10, 10)<<endl;
	}

	return 0;
}