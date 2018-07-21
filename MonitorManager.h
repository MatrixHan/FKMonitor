#pragma once
#include <string>
using namespace std;
class MonitorManager
{
public:
	static bool IsMasterRunning();
	static bool IsSlaveRunning();
	static void Master(int argc, char * argv[]);
	static void Slave(int argc, char * argv[]);
	static void Stop();
};

