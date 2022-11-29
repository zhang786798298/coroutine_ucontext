#include "process.h"
#include "mutex.h"
#include <iostream>
#include "waitgroup.h"
#include "channel.h"

int _cmain(int32_t argc, char** argv)
{
    Coroutine::WaitGroup wg;
    auto ch = new Coroutine::Channel<int>(1);

    wg.Add(1);
    Coroutine::Go([&](){
        for (int i = 0; i < 100; i++)
        {
            *ch << i;
        }
        ch->Close();
        wg.Done();
    });

    wg.Add(1);
    Coroutine::Go([&](){
        //std::cout<<"go test2"<<std::endl;
        while (!ch->IsClose())
        {
            int m = 0;
            *ch >> m;
            std::cout<< "test1:" << m << std::endl; 
        }
        wg.Done();
    });

    wg.Add(1);
    Coroutine::Go([&](){
         while (!ch->IsClose())
        {
            int m = 0;
            *ch >> m;
            std::cout<< "test2:" << m << std::endl; 
        }
        wg.Done();
    });
    wg.Wait();
    
    std::cout<<"hello test end" << std::endl;
    return 0;
}


int32_t main(int32_t argc, char** argv) 
{
    Coroutine::ProcessInstance()->SetProcesscount(0);
    Coroutine::ProcessInstance()->Start(argc, argv, _cmain);
    return 0;
}