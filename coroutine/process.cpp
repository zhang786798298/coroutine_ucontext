#include "process.h"
#include <signal.h>

namespace Coroutine
{
    Process::Process():_curindex(0), _prscnt(0)
	{

	}

	Process::~Process()
	{
		for (auto ii : _vec_prss)
		{
			delete ii;
		}
		_vec_prss.clear();
	}

    void Process::SetProcesscount(int count)
    {
        _prscnt = count;
    }

	void Process::Start(int argc, char** argv, mainFunc fn)
	{
        if (_prscnt == 0)
            _prscnt = Common::GetProcessorNum();
		for (int i = 0; i < _prscnt; i++) {
			Exector* exector = new Exector();
			exector->SetProcess(this);
            _vec_prss.push_back(exector);
		}

        //添加main函数的协程
        NewTask([&](){
            fn(argc, argv);
        });
        Init_signals();
        wait();
	}

    Task* Process::NewTask(TaskFunc fn)
    {
        Exector* exe = GetExector();
        if (!exe)
            return nullptr;
        Task* t = new Task(fn, exe);
        exe->AddTask(t);
        return t;
    }

    void Process::wait()
    {
        for (auto ii : _vec_prss)
        {
            ii->Join();
        }
    }

    void Process::Stop()
    {
        for (auto ii : _vec_prss)
        {
            ii->Quit();
        }
    }

	Exector* Process::GetExector()
	{
		if (_prscnt == 0)
			return nullptr;
		std::scoped_lock guard(_prsmtx);
		if (_curindex >= _prscnt) 
		{
			_curindex = 0;
		}
		return _vec_prss[_curindex++];
	}

	Exector* Process::FindExector(uint64_t id)
	{
        std::scoped_lock guard(_prsmtx);
		auto ii = _prss.find(id);
		if (ii == _prss.end())
			return nullptr;
		return ii->second;
	}

    void Process::RegisterExector(Exector* exe)
    {
        std::scoped_lock guard(_prsmtx);
        _prss.emplace(exe->GetID(), exe);
    }

    Task* Process::Self()
    {
        uint64_t id = Common::getCurrentThreadID();
        Exector* exe = FindExector(id);
        if (!exe)
            return nullptr;
        return exe->GetCurrent();
    }

    void Process::Init_signals()
    {
        signal(SIGHUP, signal_handle);
	    signal(SIGTERM, signal_handle);
        signal(SIGQUIT, signal_handle);
    }
    void Process::signal_handle(int signo)
    {
        switch (signo)
        {
        case SIGTERM:
        case SIGQUIT:
            ProcessInstance()->Stop();
            break;
        case SIGHUP:
            break;
        }
    }

	Process* ProcessInstance()
	{
		return Singleton<Process>::Instance();
	}

    Task* Go(TaskFunc fn)
    {
        return ProcessInstance()->NewTask(fn);
    }

    void Sleep(int64_t timeoutms)
    {
        Sleep_for(timeoutms, nullptr);
    }

    void Sleep_for(int64_t timeoutms, YieldFunc pfn)
    {
        Task* t = ProcessInstance()->Self();
        if (t)
        {
            t->Sleep(timeoutms, pfn);
        }
    }
}