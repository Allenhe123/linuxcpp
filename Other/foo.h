
#include <iostream>

class foo
{
public:
   
    foo()
    {
        std::cout<<"foo constructor\n";
    }
    
    ~foo()
    {
        std::cout<<"foo destructor\n";
    }
    
    void doit()
    {
        std::cout<<"foo doit\n";
    }
    
};
