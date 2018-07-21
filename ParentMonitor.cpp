#include "ntqueries.h"
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include "ParentMonitor.h"
#include <winternl.h>   
#include <ntstatus.h>  
#include "easylogging++.h"
#include "MonitorUtils.h"
#include "ProcessMonitor.h"
#include <chrono>
#include <thread>
#ifndef ProcessBasicInformation  
#   define ProcessBasicInformation 0  
#endif  

typedef LONG(__stdcall *PROCNTQSIP)(HANDLE, UINT, PVOID, ULONG, PULONG);

static DWORD GetParentProcessID(DWORD dwProcessId)
{
	NTSTATUS                    status = STATUS_SUCCESS;
	DWORD                       dwParentPID = DWORD(-1);
	HANDLE                      hProcess = NULL;
	PROCESS_BASIC_INFORMATION   pbi = { 0 };

	PROCNTQSIP NtQueryInformationProcess = (PROCNTQSIP)GetProcAddress(GetModuleHandle(L"ntdll"), "NtQueryInformationProcess");
	if (!NtQueryInformationProcess)
		return DWORD(-1);


	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
	if (!hProcess)
		return DWORD(-1);

	status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, (PVOID)&pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (status == STATUS_SUCCESS)
		dwParentPID = DWORD(pbi.Reserved3);

	CloseHandle(hProcess);
	LOG(INFO) << "parent id is : " << dwParentPID;
	return dwParentPID;
}

static HANDLE GetParentHandle() {
	DWORD current_id = GetCurrentProcessId();
	DWORD parent_id = GetParentProcessID(current_id);
	HANDLE parent_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, parent_id);
	return parent_handle;
}

void watch_parent_crash_by_id() {
	HANDLE parent_handle = GetParentHandle();
	if (parent_handle == NULL) {
		return ;
	}
	LOG(INFO) << "parent crashed before wait" ;
	WaitForSingleObject(parent_handle, INFINITE);
	//CloseHandle(parent_handle);
	LOG(INFO) << "parent crashed before stop" ;
	MonitorUtils::Stop();
	LOG(INFO) << "parent crashed after  stop" ;
	return ;
}
void watch_parent_crash(const wstring name) {
	LOG(INFO) << "watch_parent_crash : " << name ;
	while (ProcessMonitor::is_running(name)) {
		this_thread::sleep_for(1000ms);
	}
	LOG(INFO) << "watch_parent_crash crashed : " << name;
	MonitorUtils::Stop();
}

void ParentMonitor::start_thread(const wstring & name)
{
	thread * n = new thread(watch_parent_crash,name);
	//thread n([] {});
	//n.join();
}


//ParentMonitor::~ParentMonitor()
//{
//}
