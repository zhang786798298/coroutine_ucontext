#include "process.h"
#include <iostream>

int _cmain(int32_t argc, char** argv)
{
    std::cout<<"hello test" << std::endl;
    Coroutine::Go([=](){
        std::cout<<"go test1"<<std::endl;
        for (int i = 0; i < 100; i++)
        {
            std::cout<<"go1 : " << i << std::endl;
            Coroutine::Sleep(2);
        }
    });

    Coroutine::Go([=](){
        std::cout<<"go test2"<<std::endl;
        for (int i = 0; i < 100; i++)
        {
            std::cout<<"go2 : " << i << std::endl;
            Coroutine::Sleep(1);
        }
    });
    std::cout<<"hello test end" << std::endl;
    return 0;
}


int32_t main(int32_t argc, char** argv) 
{
    Coroutine::ProcessInstance()->SetProcesscount(4);
    Coroutine::ProcessInstance()->Start(argc, argv, _cmain);
    return 0;
}