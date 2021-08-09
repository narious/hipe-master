#ifndef TIMER_H
#define TIMER_H

#include "Timer.h"
#include <iostream>
#include <string>
#include <chrono>
#include <spdlog/spdlog.h>

/*
* This is a Timer class to be used to time functions
* Timer also takes cares of logging.
* e.g. of use
* 
*	{
*		Timer timer;
*		<Code to time here>
*	}
* 
*	Results of the timer should be configured to a file
*/

class Timer
{
	std::string name;

public:
	Timer(std::string name)
	{
		std::chrono::high_resolution_clock::now();
		this->name = name;
	}

	~Timer()
	{
		Stop();
	}

	void Stop()
	{
		auto endTimePoint = std::chrono::high_resolution_clock::now();

		auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimePoint).time_since_epoch().count();
		auto endtime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();

		auto timetaken = endtime - start;
		double ms = timetaken * 0.001;

		// TODO: Add file to recieve output of timing
		std::cout << this->name << ": " << ms << "ms" << std::endl;
        spdlog::info("Hello, {}!", "World");


	}
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;
};

#endif
