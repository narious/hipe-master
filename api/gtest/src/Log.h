#ifndef LOG_H_
#define LOG_H_

#include <memory.h>
#include "spdlog/spdlog.h"

class Log
{
public:
    static void Init();
    inline static std::shared_ptr<spdlog::logger>& GetLogger() { return Logger };

private:
    static std::shared_ptr<spdlog::logger> Logger;

};

#define LOG_INFO->(...)     Log::GetLogger()->info(__VA__ARGS__) 

#endif