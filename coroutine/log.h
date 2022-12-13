#pragma once
#include <sstream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include "mutex.h"
#include "task.h"

namespace Coroutine 
{
    enum LogLevel 
    {
        NONE_LOG = 1,
        ERROR_LOG = 2,
        DEBUG_LOG = 3,
        INFO_LOG = 4
    };

    static std::unordered_map<int, std::string> levelmap = 
    {
        {LogLevel::NONE_LOG, "[NONE]"},
        {LogLevel::ERROR_LOG, "[ERROR]"},
        {LogLevel::DEBUG_LOG, "[DEBUG]"},
        {LogLevel::INFO_LOG, "[INFO]"},
    };

    class log 
    {
    public:
        log();
        ~log();

        virtual bool Initialize(const std::string& filePath, int logLevel = INFO_LOG, bool sync = false);
        virtual bool Write(const std::string& log, int level);

        bool newfile();

    private:
        std::string _path;
        int _logLevel;
        time_t _curlogTime;
        Mutex _mtx;
        std::ofstream _logstream;
        std::vector<std::string> _pending;
        Task* _logtask;
        Condition_variable _cv;
    };

    log *LogInstance();
}

#define LOG_FORMAT_PREFIX "[" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "]" << "|"
#define LOG_TIME(ss) {auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());ss<<std::put_time(std::localtime(&time), "[%Y-%m-%d %H:%M:%S]");}
#define LOGMSG(level,msg...) do{std::stringstream ss;LOG_TIME(ss);ss<<Coroutine::levelmap[level]<<LOG_FORMAT_PREFIX<<msg<<std::endl;Coroutine::LogInstance()->Write(ss.str(), level);}while(0)
#define LOGINFO(msg...) LOGMSG(Coroutine::INFO_LOG,msg)
#define LOGDEBUG(msg...) LOGMSG(Coroutine::DEBUG_LOG,msg)
#define LOGERROR(msg...) LOGMSG(Coroutine::ERROR_LOG,msg)