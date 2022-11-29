#pragma once
#include "mutex.h"
#include <unordered_map>
#include <thread>
#include <ucontext.h>
#include <functional>
#include "../common/timer.h"

namespace Coroutine 
{
    using YieldFunc = std::function<void()>;
    class Task;
    class Process;
    class Exector
    {
    public:
        Exector();
        virtual ~Exector();
        using TaskQueue = std::list<Task*>;

        int AddTask(Task* t);
        void FiniTask(Task* t);
        void Yield(Task* t, YieldFunc prefn = nullptr);
        void Resume(Task* t);
        void Sleep(Task* t, uint64_t timeoutms, YieldFunc prefn = nullptr);
        void QuitSleep(Task* t);

        void Run();
        void schedule();
        void switchto(ucontext_t* dest, YieldFunc prefn = nullptr);
        

        ucontext_t* GetMainHandle() {return _mainHandle;}
        ucontext_t* GetCurHandle() {return _curHandle;}
        uint64_t GetID() {return _id;}
        Task* GetCurrent() {return _current;}
        void SetProcess(Process* p) {_prss = p;}

        void Quit()
        {
            is_active.store(false, std::memory_order_relaxed);
        }
        void Join()
        {
            if (work_thread.joinable())
                work_thread.join();
        }
    private:
        Spinlock _listlock;
        uint64_t _id;
        TaskQueue _qtraw;
        TaskQueue _qready;
        TaskQueue _qfini;
        ucontext_t* _mainHandle{nullptr};
        ucontext_t* _curHandle{nullptr};
        Task* _current;
        std::condition_variable _cv;
        std::mutex _lock;
        std::atomic<bool> is_active;
		std::thread work_thread;
        Common::Timer* _timer;
        Process* _prss;
    };
}