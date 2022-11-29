#pragma once
#include <string.h>
#include <thread>
#include <sstream>
#include <chrono>

#if defined(__linux__)
// 分支预测
	#define likely(x)       __builtin_expect( (x), 1 )
	#define unlikely(x)     __builtin_expect( (x), 0 )
#else
	#define likely(x)		x
	#define unlikely(x)		x
#endif

namespace Common
{
	inline uint64_t getCurrentThreadID() 
    {  
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        std::string stid = oss.str();
        return static_cast<uint64_t>(std::stoul(stid));
    }

    inline int GetProcessorNum()
	{
		int kProcessorNum = 0;
		FILE* fstream = NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		if (NULL == (fstream = popen("grep 'processor' /proc/cpuinfo | sort -u | wc -l", "r")))
		{
			return -1;
		}

		fgets(buff, sizeof(buff), fstream);
		kProcessorNum = atoi(buff);

		pclose(fstream);
		return kProcessorNum;
	}

    inline std::chrono::milliseconds Nowms()
    {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::steady_clock;
        auto now = steady_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch());
        return ms;
    }
}