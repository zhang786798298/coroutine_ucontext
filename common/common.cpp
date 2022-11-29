#include "common.h"

namespace Common
{
    uint64_t getCurrentThreadID() 
    {  
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        std::string stid = oss.str();
        return static_cast<uint64_t>(std::stoul(stid));
    }

    int GetProcessorNum()
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
}