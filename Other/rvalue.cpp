#include <iostream>

class Foo
{
public:
    Foo() { std::cout<<"constructor"<<std::endl;}
    ~Foo() {std::cout<<"destructor"<<std::endl;}
    void doit() {std::cout<<"Foo - doit"<<std::endl;}
};


void Fun(Foo f)
{
    f.doit();   
}

int main()
{
    Foo&& ff = Foo();
    
    Fun(ff);
    
    return 0;
}