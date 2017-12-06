#include <string>
#include <algorithm>
#include <iostream>

struct strCmp
{
	bool operator () (char x, char y)
	{
		return x == ' ' && y == ' ';
	}

};

void removeContinuousSpaces(std::string& str)
{
	std::string::iterator last = std::unique(str.begin(), str.end(), strCmp());
	str.erase(last, str.end());
}


int main()
{
	std::string strTest("allen  he  is       great!     ");
	std::cout<<strTest<<std::endl;
	removeContinuousSpaces(strTest);
	std::cout<<strTest<<std::endl;

	return 0;
}