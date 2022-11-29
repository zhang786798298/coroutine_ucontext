#pragma once
#include "mutex.h"
namespace Coroutine 
{
    class WaitGroup
    {
    public:
        WaitGroup();
        ~WaitGroup() = default;

        void Add(int count);
        void Done();
        void Wait();
    private:
        std::atomic<int> _count;
        Spinlock _tlock;
        std::list<Task*> _lstTasks;
    };
}