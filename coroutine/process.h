#pragma once
#include "exector.h"
#include "task.h"
#include "Singleton.h"
#include <vector>

namespace Coroutine
{
    using mainFunc = std::function<int(int,char**)>;
    class Process
    {
    public:
        Process();
        ~Process();
        void Start(int argc, char** argv, mainFunc fn);

        Task* NewTask(TaskFunc fn);
        void wait();
        void Stop();
        void SetProcesscount(int count);

		Exector* GetExector();
		Exector* FindExector(uint64_t id);
        void RegisterExector(Exector* exe);
        Task* Self();  

        //信号量
        void Init_signals();
        static void signal_handle(int signo);
	private:

		int _curindex;
		int _prscnt;
		std::mutex _prsmtx;
		std::unordered_map<uint64_t, Exector*> _prss;
        std::vector<Exector*> _vec_prss;
    };

    Process* ProcessInstance();

    Task* Go(TaskFunc fn);

    void Sleep(int64_t timeoutms);

    void Sleep_for(int64_t timeoutms, YieldFunc pfn);
}