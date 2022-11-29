#include "task.h"

namespace Coroutine 
{
    static std::atomic<uint64_t> TaskID{0};

    Task::Task(TaskFunc fn, Exector* exe)
    {
        _id = TaskID.fetch_add(1, std::memory_order_relaxed);
        _fn = fn;
        _exec = exe;
    }

    Task::~Task()
    {
        if (_stack != nullptr) 
        {
            free(_stack);
            _stack = nullptr;
        }
        if (nullptr != _handle) 
        {
            delete _handle;
            _handle = nullptr;
        }
    }

    void Task::Init()
    {
        _handle = new ucontext_t;
        _stack = (char*)malloc(TASK_STACK_SIZE);
        getcontext(_handle);
        _handle->uc_stack.ss_sp = _stack;
        _handle->uc_stack.ss_size = TASK_STACK_SIZE;
        _handle->uc_stack.ss_flags = 0;
        _handle->uc_link = _exec->GetMainHandle();  //上下文切到 调度器
        auto *ptr = reinterpret_cast<void *>(this);
        auto value = reinterpret_cast<std::uint64_t>(ptr);
        if (sizeof(void *) == sizeof(uint32_t)) 
        {
            makecontext(_handle, reinterpret_cast<void (*)()>(&Task::_entry32), 1, static_cast<uint32_t>(value));
        } 
        else 
        {
            makecontext(_handle, reinterpret_cast<void (*)()>(&Task::_entry64), 2, 
            static_cast<uint32_t>((0xFFFFFFFF00000000 & value) >> 32),
            static_cast<uint32_t>(0x00000000FFFFFFFF & value));
        }
    }
    void Task::Resume()
    {
        _exec->Resume(this);
    }
    void Task::Yield(YieldFunc pfn)
    {
        _exec->Yield(this, pfn);
    }
    
    void Task::Sleep(int64_t timeoutms, YieldFunc pfn)
    {
        _exec->Sleep(this, timeoutms, pfn);
    }
    void Join()
    {

    }

    void Task::_entry(void* p)
    {
        auto *t = reinterpret_cast<Task*>(p);
        if (t->_fn) {
            t->_fn();
        }
        t->_exec->FiniTask(t);
    }
    void Task::_entry32(uint32_t param)
    {
        _entry(reinterpret_cast<void *>(static_cast<std::uint64_t>(param))); 
    }
    void Task::_entry64(uint32_t param1, uint32_t param2)
    {
        std::uint64_t param = 0;
        param = 0x00000000FFFFFFFF & static_cast<std::uint64_t>(param1);
        param <<= 32;
        param |= 0x00000000FFFFFFFF & static_cast<std::uint64_t>(param2);
        _entry(reinterpret_cast<void *>(param));
    }
}