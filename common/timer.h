#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <list>

namespace Common
{
    using TimerEventFunc = std::function<void()>;
    using TaskTimeout = std::chrono::milliseconds;
    using TaskTimeoutMic = std::chrono::microseconds;


    class Timer{
        struct _timerevent {
            uint64_t id;
            TimerEventFunc fn;
            TaskTimeout timeoutms;
            TaskTimeout exp;
            bool loop{ true };
        };

    public:
        Timer() = default;
        virtual ~Timer() {
            Stop();
        }
        std::chrono::milliseconds Nowms();
        virtual int Loop();
        virtual uint64_t Add(TimerEventFunc efn, uint64_t timeoutms, bool loop = true);
        virtual int Remove(uint64_t id);
        virtual int Stop() {
            return 0;
        }

    protected:
        void insertEvent(const _timerevent& event);

    private:
        std::list<_timerevent> _eventlist;
        std::atomic<uint64_t> _id{ 0 };
    };

    Timer* TimerInstance();
}