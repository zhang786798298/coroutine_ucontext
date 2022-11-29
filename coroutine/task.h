#pragma once
#include "exector.h"

#define TASK_STACK_SIZE 256 * 1024
using TaskFunc = std::function<void()>;

namespace Coroutine 
{
    enum class TaskStatus : std::uint32_t 
    {
        None = 0,
        Ready,
        Suspend,
        Fini,
    };

    //static std::atomic<uint64_t> TaskID;

    class Task
    {
    public:
        Task(TaskFunc fn, Exector* exe);
        virtual ~Task();

        void Init();
        void Resume();
        void Yield(YieldFunc pfn = nullptr);
        void Sleep(int64_t timeoutms, YieldFunc pfn);
        void SetSleepID(uint64_t id) {_Sleepid = id;}
        uint64_t GetSleepID() {return _Sleepid;}
        void QuitSleep() {_exec->QuitSleep(this);}
        void Join();

        void Quit() {_active.store(false, std::memory_order_relaxed);}
        bool IsQuit() {return _active.load(std::memory_order_relaxed) == false;}
        void ResetExec(Exector* exe) {_exec = exe;}
        void SetStatus(TaskStatus s) {_status.store(s, std::memory_order_relaxed);}
        bool ChangeStatus(TaskStatus& expected,TaskStatus s)
        {
            return _status.compare_exchange_strong(expected, s, std::memory_order_acquire);
        }

        bool IsFinish() {return _status.load(std::memory_order_relaxed) == TaskStatus::Fini;}
        uint64_t GetID() {return _id;}
        TaskStatus GetStatus() {return _status.load(std::memory_order_relaxed);}
        ucontext_t* GetHandle() {return _handle;}
        Exector* GetExector() {return _exec;}
    private:
        static void _entry(void* p);
        static void _entry32(uint32_t param);
        static void _entry64(uint32_t param1, uint32_t param2);

    private:
        uint64_t _id;
        uint64_t _Sleepid;
        char *_stack;
        std::atomic<TaskStatus> _status{TaskStatus::None};
        std::atomic<bool> _active{true};
        TaskFunc _fn;
        ucontext_t* _handle{nullptr};
        Exector* _exec;
    };
}