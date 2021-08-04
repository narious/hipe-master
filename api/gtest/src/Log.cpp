#include "Log.h"

std::shared_ptr<spdlog::logger> Log::Logger;

void Log::Init()
{
    spdlog::set_pattern()
}