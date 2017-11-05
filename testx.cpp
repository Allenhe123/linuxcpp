
#include <iostream>

class inventory
{
	public:
		void print() const __attribute__ ((noinline))
		{
			std::cout<<"hello world"<<std::endl;
			return;			
		}

		void print2() const __attribute__ ((inline))
		{
			std::cout<<"hello world2"<<std::endl;
			return;			
		}
};


int main()
{
	inventory in;
	in.print();
	in.print2();


	int x = -11;
	std::cout<<x % 2<<std::endl;

	return 0;
}