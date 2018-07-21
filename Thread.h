#pragma once
class Thread
{
public:
	static void MonitorProcess(int argc, char * argv[]);
	static void WaitStop();
	static void MonitorSelf();
};

