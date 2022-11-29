#include "mutex.h"
#include "task.h"
#include "process.h"


namespace Coroutine
{
    CMutex::CMutex() : _count(0)
    {

    }
    void CMutex::Lock()
    {
        if (likely(TryLock()))
        {
            return;
        }
        std::unique_lock guard(_tlock);
        if (_count.fetch_add(1, std::memory_order_acquire) == 0)
            return;
        //添加到解锁队列
        Task* current = ProcessInstance()->Self();

        current->Yield([&](){
            _lstTasks.push_back(current);
            guard.unlock();
        });
    }
    void CMutex::UnLock()
    {
        if (_count.fetch_sub(1, std::memory_order_release) == 1)
            return;
        //从解锁队列恢复一个：
        std::unique_lock guard(_tlock);
        Task* t = _lstTasks.front();
        _lstTasks.pop_front();
        guard.unlock();
        t->Resume();
    }
    bool CMutex::TryLock()
    {
        uint32_t expected = 0;
        return _count.compare_exchange_strong(expected, 1, std::memory_order_acquire);
    }

    Condition_variable::Condition_variable()
    {

    }
    Condition_variable::~Condition_variable()
    {

    }

    void Condition_variable::wait(std::unique_lock<Mutex>& lck)
    {
        Task* current = ProcessInstance()->Self();
        current->Yield([&](){
            std::unique_lock guard(_tlock);
            _lstTasks.push_back(current);
            lck.unlock();
        });
        lck.lock();
    }
    std::cv_status Condition_variable::wait_for(std::unique_lock<Mutex> &lck, uint64_t timeoutms)
    {
        auto timeout = Common::Nowms() + std::chrono::milliseconds(timeoutms);
        Task* current = ProcessInstance()->Self();

        current->Sleep(timeoutms, [&](){
            std::unique_lock guard(_tlock);
            _lstTasks.push_back(current);
            lck.unlock();
        });
        lck.lock();
        //恢复，可能是timeout 需要删掉 _lstTasks
        std::unique_lock guard(_tlock);
        _lstTasks.remove(current);
        return Common::Nowms() >= timeout ? std::cv_status::timeout : std::cv_status::no_timeout;
    }
    std::cv_status Condition_variable::wait_until(std::unique_lock<Mutex> &lck, std::chrono::milliseconds time)
    {
        auto now = Common::Nowms();
        if (now >= time) 
        {
            return std::cv_status::timeout;
        }
        return wait_for(lck, (time-now).count());
    }
    void Condition_variable::notify_one()
    {
        std::unique_lock guard(_tlock);
        if (_lstTasks.empty())
            return;
        Task* t = _lstTasks.front();
        _lstTasks.pop_front();
        guard.unlock();
        //可能有wait_for()
        t->QuitSleep();
        t->Resume();
    }
    void Condition_variable::notify_all()
    {
        while(!_lstTasks.empty()) 
        {
            notify_one();
        }
    }
}