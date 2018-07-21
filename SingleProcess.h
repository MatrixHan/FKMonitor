#pragma once
#include <windows.h> 

//This code is from Q243953 in case you lose the article and wonder
//where this code came from.
class SingleProcess
{
protected:
	DWORD  m_dwLastError;
	HANDLE m_hMutex;

public:
	SingleProcess(TCHAR *strMutexName);
	~SingleProcess();
	bool IsAnotherInstanceRunning();

};
