#include "process.h"
#include "mutex.h"
#include <iostream>
#include "waitgroup.h"

int _cmain(int32_t argc, char** argv)
{
    Coroutine::Mutex mu;
    Coroutine::Condition_variable cv;
    Coroutine::WaitGroup wg;
    int m = 0;
    int c = 0;

    wg.Add(1);
    Coroutine::Go([&](){
        //std::cout<<"go test1"<<std::endl;
        for (int i = 0; i < 100; i++)
        {
            std::unique_lock guard(mu);
            //std::cout<<"go1 : " << m++ << std::endl;
        }
        wg.Done();
    });

    wg.Add(1);
    Coroutine::Go([&](){
        //std::cout<<"go test2"<<std::endl;
        for (int i = 0; i < 100; i++)
        {
            std::unique_lock guard(mu);
            //std::cout<<"go2 : " << m++ << std::endl;
        }
        wg.Done();
    });

    wg.Add(1);
    Coroutine::Go([&](){
        for (int i = 0; i <= 100; i++)
        {
            Coroutine::Sleep(1);
            std::unique_lock guard(mu);
            c++;
            cv.notify_one();
        }
        wg.Done();
    });
    wg.Add(1);
    Coroutine::Go([&](){
        while (c < 100)
        {
            std::unique_lock guard(mu);
            cv.wait(guard);
            std::cout<<"go3 : " << c << std::endl;
        }
        wg.Done();
    });
    wg.Add(1);
    Coroutine::Go([&](){
        while (c < 100)
        {
            std::unique_lock guard(mu);
            cv.wait(guard);
            std::cout<<"go4 : " << c << std::endl;
        }
        wg.Done();
    });
    wg.Add(1);
    Coroutine::Go([&](){
        while (c < 100)
        {
            std::unique_lock guard(mu);
            if (cv.wait_for(guard, 5) == std::cv_status::timeout)
            {
                std::cout<<"go5 : timeout" << std::endl;
                continue;
            }
            std::cout<<"go5 : " << c << std::endl;
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