#include "log.h"
#include "process.h"
#include <iostream>
#include "Singleton.h"


namespace Coroutine 
{
    log::log():_curlogTime(0),_logtask(nullptr)
    {

    }

    log::~log()
    {
        if (_logtask)
            _logtask->Quit();
    }
    bool log::newfile()
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (Common::IsSameDay(now, _curlogTime))
            return true;
        _curlogTime = now;
        std::stringstream ss;
        ss << _path << "/" << "log_" << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S") << ".log";
        if (_logstream.is_open()) 
        {
            _logstream.close();
        }
        _logstream.open(ss.str().c_str(), std::ios::out);
        return _logstream.is_open();
    }

    bool log::Initialize(const std::string& filePath, int logLevel, bool sync)
    {
        _path = filePath;
        _logLevel = logLevel;
        if (newfile() == false) 
        {
            std::cout << "create new log file failed!!" << std::endl;
            return false;
        }
        if (!sync)
        {
            _logtask = Go([=](){
                while (true)
                {
                    std::unique_lock lck(_mtx);
                    _cv.wait(lck);
                    std::vector<std::string> loglist;
                    if (_pending.size() > 0) 
                    {
                        loglist.swap(_pending);
                    }
                    lck.unlock();
                    if (!newfile())
                        break;
                    for (size_t i = 0; i < loglist.size(); ++i) 
                    {
                        _logstream << loglist[i];
                    }
                    if (loglist.size() != 0) 
                    {
                        _logstream.flush();
                    }
                }
            });
        }
        return true;
    }

    bool log::Write(const std::string& log, int level)
    {
        if (level > _logLevel && _logLevel != 0) 
            return false;
        if (_logtask == nullptr)
        {
            //同步写
            if (!newfile())
                return false;
            _logstream << log;
            _logstream.flush();
        }
        else
        {
            //异步写
            {
                std::scoped_lock guard(_mtx);
                _pending.push_back(log);
            }
            _cv.notify_one();
        }
        return true;
    }

    log* LogInstance() 
    {  
        return Singleton<log>::Instance();
    }
}