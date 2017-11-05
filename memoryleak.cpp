#include <cstdlib>

void fun()
{
	int *p = new int[5];
	p[0] = 0;
	p[5] = 5;
}

int main(int argc, char* argv[])
{
    fun();
    return 0;
}