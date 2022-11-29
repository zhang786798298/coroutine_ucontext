#include "timer.h"
#include "Singleton.h"
#include "common.h"
#include <algorithm>

namespace Common {

    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    int Timer::Loop() {
        while(!_eventlist.empty()) {
            auto timerevent = _eventlist.front();
            auto nowms = Common::Nowms();
            if (nowms < timerevent.exp) {
                return 0;
            }
            _eventlist.pop_front();
            timerevent.fn();
            if (timerevent.loop) {
                timerevent.exp = Common::Nowms() + timerevent.timeoutms;
                insertEvent(timerevent);
            }
        }
        return 0;
    }

    void Timer::insertEvent(const _timerevent& event) {
        auto iter = std::find_if(_eventlist.begin(), _eventlist.end(), [=](_timerevent tr) {
            if (tr.exp >= event.exp) {
                return true;
            }
            return false;
            });
        if (iter != _eventlist.end()) {
            _eventlist.insert(iter, event);
        }
        else {
            _eventlist.push_back(event);
        }
    }

    uint64_t Timer::Add(TimerEventFunc efn, uint64_t timeoutms, bool loop) {
        auto now = steady_clock::now();
        TaskTimeout exp = duration_cast<milliseconds>(now.time_since_epoch()) + TaskTimeout(timeoutms);
        _timerevent event{ ++_id, efn, TaskTimeout(timeoutms), exp, loop };
        insertEvent(event);
        return _id;
    }

    int Timer::Remove(uint64_t id) {
        auto iter = std::find_if(_eventlist.begin(), _eventlist.end(), [=](_timerevent tr) {
            if (id == tr.id) {
                return true;
            }
            return false;
            });
        if (iter != _eventlist.end()) {
            _eventlist.erase(iter);
        }
        return 0;
    }


    Timer* TimerInstance()
    {
        return Singleton<Timer>::Instance();
    }

} // namespace Common