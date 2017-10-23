#include <fstream>
#include <vector>
#include <string>
#include <iostream>

void readfile(const std::string& filename, std::vector<std::string>& out)
{
	std::ifstream ifs;
	ifs.open(filename.c_str());
	if (ifs.fail())
		return;

	char str[256];
	while (ifs.getline(str, 256))
	{
		out.push_back(str);
	}
}

int main(int argc, char** argv)
{
	if (argc <2 )
		return -1;
	
	char* filename = argv[1];
	std::vector<std::string> vec;
	readfile(filename, vec);
	for (int i=0; i<vec.size(); ++i)
		std::cout<<vec[i]<<std::endl;

	return 0;
}


