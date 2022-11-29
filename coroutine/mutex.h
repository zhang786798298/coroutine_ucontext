#pragma once
#include <atomic>
#include <list>
#include <mutex>
#include <condition_variable>
#include "../common/common.h"

namespace Coroutine 
{
    class Spinlock
    {
    public:
        Spinlock() = default;
        ~Spinlock() = default;

        void lock() 
        {
            if (likely(try_lock()))
            {
                return;
            }
            do 
            {
                // test lock
                while (_locked.load(std::memory_order_relaxed));
                //set finally
            } while (_locked.exchange(true, std::memory_order_acquire));
        }

        bool try_lock()
        {
            return !_locked.exchange(true, std::memory_order_acquire);
        }

        void unlock()
        {
            _locked.store(false, std::memory_order_release);
        }

    private:
        std::atomic<bool> _locked{false};
    };

    class Task;
    class CMutex
    {
    public:
        CMutex();
        virtual ~CMutex() = default;
        virtual void Lock();
        virtual void UnLock();
        virtual bool TryLock();
    private:
        std::atomic<uint32_t> _count;
        Spinlock _tlock;
        std::list<Task*> _lstTasks;
    }; 

   //参照 std::mutex lock替换成协程的Yeild
    class Mutex
    {
    public:
        Mutex()
        {
            mu = new CMutex;
        }
        virtual ~Mutex()
        {
            delete mu;
        }
        void lock() 
        {
            mu->Lock();
        }
        void unlock() 
        {
            mu->UnLock();
        }
        bool try_lock() 
        {
            return mu->TryLock();
        }
    private:
        CMutex* mu;
    };

    class Condition_variable
    {
    public:
        Condition_variable();
        virtual ~Condition_variable();

        void wait(std::unique_lock<Mutex>& lck);
        std::cv_status wait_for(std::unique_lock<Mutex> &lck, uint64_t timeoutms);
        std::cv_status wait_until(std::unique_lock<Mutex> &lck, std::chrono::milliseconds time);
        void notify_one();
        void notify_all();
    private:
        Spinlock _tlock;
        std::list<Task*> _lstTasks;
    };
}