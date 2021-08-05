#include "Log.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

Log::Log(const char* name, const char* logfile){
    // Setting up where to log
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    // Specifiy logile, file and file size
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile, true));

    auto Logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
    _Logger = Logger;
    //register it if you need to access it globally
    spdlog::register_logger(Logger);
};

Log::~Log(){};
