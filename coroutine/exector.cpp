#include "exector.h"
#include "task.h"
#include "process.h"
#include "../common/common.h"

namespace Coroutine
{
    Exector::Exector()
    {
        _timer = new Common::Timer();
        is_active.store(true, std::memory_order_relaxed);
        work_thread = std::thread(&Exector::Run, this);
    }
    Exector::~Exector()
    {
        if (_timer)
        {
            delete _timer;
            _timer = nullptr;
        }
        if (_mainHandle)
        {
            delete _mainHandle;
            _mainHandle = nullptr;
        }
        //清空tasks
        for (auto ii : _qtraw)
            delete ii;
        for (auto ii : _qready)
            delete ii;
        for (auto ii : _qfini)
            delete ii;
    }
    int Exector::AddTask(Task* t)
    {
        {
            std::scoped_lock<Spinlock> guard(_listlock);
            _qtraw.push_back(t);
        }
        _cv.notify_one();
        return 0;
    }
    void Exector::FiniTask(Task* t)
    {
        t->SetStatus(TaskStatus::Fini);
        _qfini.push_back(t);
    }
    void Exector::Yield(Task* t, YieldFunc prefn)
    {
        if (unlikely(_current != t))
            return;
        //改变状态
        TaskStatus expected = TaskStatus::Ready;
        t->ChangeStatus(expected, TaskStatus::Suspend);
        //切换 上下文
        _current = nullptr;
        switchto(_mainHandle, prefn);
    }
    void Exector::Resume(Task* t)
    {
        if (unlikely(_current == t))
            return;
        TaskStatus expected = TaskStatus::Suspend;
        if (t->ChangeStatus(expected, TaskStatus::Ready))
        {
            //从暂停 到 就绪状态
            std::scoped_lock guard(_listlock);
            _qready.push_back(t);
            _cv.notify_one();
        }
    }
    void Exector::Sleep(Task* t, uint64_t timeoutms, YieldFunc prefn)
    {
        if (unlikely(_current != t))
            return;
        if (timeoutms == 0)
        {
            {
                std::scoped_lock guard(_listlock);
                _qready.push_back(t);
            }
            //切换 上下文
            _current = nullptr;
            switchto(_mainHandle, prefn);
            return;
        }
        uint64_t id = this->_timer->Add([=](){
                t->SetSleepID(0);
                t->Resume();
            }, timeoutms, false);
        t->SetSleepID(id);
        Yield(t, prefn);
    }

    void Exector::QuitSleep(Task* t)
    {
        if (t->GetSleepID() == 0 )
            return;
        _timer->Remove(t->GetSleepID());
        t->SetSleepID(0);
    }
    void Exector::switchto(ucontext_t* dest, YieldFunc prefn)
    {
        if (unlikely(dest == _curHandle))
            return;
        auto src = _curHandle;
        _curHandle = dest;
        if (prefn != nullptr)
            prefn();
        swapcontext(src, _curHandle);
    }
    void Exector::Run()
    {
        const int MAX_WAIT_TIMEOUT_MIC = 1000;
        const int MIN_WAIT_TIMEOUT_MIC = 300;
        int cvtimeout = MIN_WAIT_TIMEOUT_MIC;

        _id = Common::getCurrentThreadID();
        _prss->RegisterExector(this);
        _mainHandle = new ucontext_t;
        _curHandle = _mainHandle;

        while (is_active.load(std::memory_order_relaxed))
		{
			//清除 结束的协程
            if (!_qfini.empty())
            {
                for (auto ii : _qfini)
                {
                    delete ii;
                }
                _qfini.clear();
            }
            //调度线程
            schedule();
            _timer->Loop();
            //sleep 等待
            if (_qtraw.empty() && _qready.empty()) 
            {
                std::unique_lock<std::mutex> sync(_lock);
                if (_cv.wait_for(sync,std::chrono::microseconds(cvtimeout)) == std::cv_status::timeout) 
                {
                    cvtimeout = (cvtimeout >=  MAX_WAIT_TIMEOUT_MIC ? MAX_WAIT_TIMEOUT_MIC : cvtimeout + 10);
                } 
                else 
                {
                    cvtimeout = MIN_WAIT_TIMEOUT_MIC;
                }
            }
		}
    }

    void Exector::schedule()
    {
        if (!_qtraw.empty())
        {
            Task* task = nullptr;
            {
                std::unique_lock guard(_listlock);
                task = _qtraw.front();
                _qtraw.pop_front();
            }
            task->Init();
            task->SetStatus(TaskStatus::Ready);
            {
                std::unique_lock guard(_listlock);
                _qready.push_back(task);
            }
        }
        while (!_qready.empty())
        {
            {
                std::scoped_lock<Spinlock> guard(_listlock);
                _current = _qready.front();
                _qready.pop_front();
            }
            if (_current->IsQuit())
            {
                FiniTask(_current);
            }
            else
            {
                //切换 上下文
                switchto(_current->GetHandle());
            }
            _current = nullptr;
            _curHandle = _mainHandle;
        }
    }

}