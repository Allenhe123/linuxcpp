
#include <iostream>
#include <memory>

class foo
{
    public:
        foo() { std::cout<<"constructor"<<std::endl;}
        ~foo() { std::cout<<"destructor"<<std::endl; }

        void doit() { std::cout<<"do it"<<std::endl; }
};

int main()
{
    std::unique_ptr<foo> sf(new foo);
    sf->doit();
    (*sf).doit();

    sf.reset(new foo);
    sf->doit();

    return 0;
}

