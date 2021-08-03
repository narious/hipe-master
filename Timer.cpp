
#include <iostream>
#include <chrono>

/*
* This is a Timer class to be used to time functions
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
public:
	Timer()
	{
		std::chrono::high_resolution_clock::now();
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
		std::cout << ms << "ns" << std::endl;


	}
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimePoint;
};