#ifndef LOG_H_
#define LOG_H_

#include <memory.h>
#include "spdlog/spdlog.h"

class Log
{
public:
    Log(const char* name, const char* logfile){};
    ~Log(){};
    
    inline std::shared_ptr<spdlog::logger>& GetLogger() { return _Logger; };

private:
    std::shared_ptr<spdlog::logger> _Logger;
};

#endif