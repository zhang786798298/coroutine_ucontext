#include "waitgroup.h"
#include "process.h"

namespace Coroutine 
{
    WaitGroup::WaitGroup():_count(0)
    {

    }

    void WaitGroup::Add(int count)
    {
        _count.fetch_add(count, std::memory_order_relaxed);
        if (_count.load(std::memory_order_acquire) == 0) 
        {
            std::unique_lock guard(_tlock);
            for (auto ii : _lstTasks)
            {
                ii->Resume();
            }
        }
    }

    void WaitGroup::Done()
    {
        Add(-1);
    }
    void WaitGroup::Wait()
    {
        Task* current = ProcessInstance()->Self();
        if (_count.load(std::memory_order_acquire) != 0)
        {
            current->Yield([&](){
                std::unique_lock guard(_tlock);
                _lstTasks.push_back(current);
            });
        }
    }
}