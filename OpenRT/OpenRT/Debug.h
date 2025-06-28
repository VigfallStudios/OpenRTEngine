/*
	OpenRT
	Debug.h
	Header file for Debugging
*/

#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>

class DebugLogger
{
public:
	static void Log(const std::string& msg)
	{
		std::cout << msg << std::endl;
	}

	static void LogError(const std::string& msg)
	{
		std::cout << "[Error] " << msg << std::endl;
	}

	static void LogWarning(const std::string& msg)
	{
		std::cout << "[Warning] " << msg << std::endl;
	}
};

extern DebugLogger Debug;

#endif