#include "ProcessMonitor.h"
#include "ntqueries.h"
#include <future>
#include "easylogging++.h"
#include "FileUtils.h"
#include <codecvt>
#include <Tlhelp32.h>
#include <winbase.h>
#include "MonitorUtils.h"

ProcessMonitor::ProcessMonitor(uint32_t pid) : m_state{ ProcessState::Running } {
	m_handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (m_handle == nullptr) {
		err("Cannot attach to a process");
	}
	if (!get_cmd_line_from_process()) {
		err("Cannot get CLI arguments from attached process");
	}

	if (!RegisterWaitForSingleObject(&m_process_exit_wait_handle, m_handle, ProcessExitedCallback,
		this, INFINITE, WT_EXECUTEONLYONCE)) {
		err("Cannot subscribe for process termination");
	}
}

ProcessMonitor::ProcessMonitor(const std::wstring& path, const std::wstring& args) :
	m_cmd(path + L' ' + args), m_state{ ProcessState::Stopped } {
	start_process();
}

ProcessMonitor::ProcessMonitor(const std::string& path, const std::string& args) :
	m_cmd(MonitorUtils::s2ws(path) + L' ' + MonitorUtils::s2ws(args)), m_state{ ProcessState::Stopped } {
	start_process();
}

uint32_t ProcessMonitor::get_pid() {
	auto lock = m_lock.LockShared();
	return GetProcessId(m_handle);
}

HANDLE ProcessMonitor::get_handle() {
	auto lock = m_lock.LockShared();
	return m_handle != nullptr ? m_handle : INVALID_HANDLE_VALUE;
}

ProcessMonitor::ProcessState ProcessMonitor::get_state() {
	auto lock = m_lock.LockShared();
	return m_state;
}

bool ProcessMonitor::stop_process(uint32_t error_code) {
	auto lock = m_lock.LockExclusive();
	if (m_handle != nullptr) {
		m_state = ProcessState::Stopped;
		auto success = ProcessMonitor::kill_process(get_name());//TerminateProcess(m_handle, error_code);
		m_handle = nullptr;
		if (success) {
			log("Process manually stopped");
			run_callbacks(m_on_proc_manually_stopped_callbacks);
		}
		return success==TRUE;
	}
	return false;
}

static bool create_process(const std::wstring exeCmd) {
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	// Do nothing if allocation fails - exception just bubbles up
	// No cleanup needed
	if (!CreateProcess(
		0,
		//L"DNSMonitor.exe",
		LPWSTR(exeCmd.c_str()),
		0, 0, FALSE, CREATE_NO_WINDOW, 0, 0,
		&si,
		&pi)) {
		LOG(ERROR) << "start master process err";
		return false;
	}
	return true;
}
bool ProcessMonitor::is_running(const wstring & exeName) {
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	bool found = false;
	while (hRes)
	{
		//LOG(INFO) << "processs [" << MonitorUtils::ws2s(pEntry.szExeFile) << "] ....";
		wstring curr = std::wstring(pEntry.szExeFile);
		//if (std::wstring(pEntry.szExeFile) == MonitorUtils::s2ws(exeName)) {
		if (curr == exeName) {
			found = true;
			break;
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
	return found;
}
bool ProcessMonitor::kill_process(const std::wstring & exeName) {
	wstring exeNameTrim = exeName;
	exeNameTrim.erase(exeNameTrim.find_last_not_of(L" \n\r\t") + 1);
	LOG(INFO) << "kill process [" << exeNameTrim << "] begin";
	// dnsmodifier 通过stop 优雅结束并恢复dns配置
	// dnsserver 不响应stop命令，由下面的terminate函数结束, 但是执行一次没有副作用,故统一处理
	create_process(exeNameTrim + wstring(L" stop"));
	for (int i = 0; i < 10; i++) {
		if (!is_running(exeNameTrim)) {
			break;
		}
		Sleep(500);
	}
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	while (hRes)
	{
		//LOG(INFO) << "processs [" << MonitorUtils::ws2s(pEntry.szExeFile) << "] ....";
		if (std::wstring(pEntry.szExeFile) == exeNameTrim)
		{
			//LOG(INFO) << "processs [" << MonitorUtils::ws2s(pEntry.szExeFile) << "] pid " << pEntry.th32ProcessID;
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
				(DWORD)pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				LOG(INFO) << "process terminate [" << MonitorUtils::ws2s(pEntry.szExeFile) << "] pid " << pEntry.th32ProcessID;
				BOOL bOkey = TerminateProcess(hProcess, 0);
				if (bOkey)
				{
					WaitForSingleObject(hProcess, 1000);
					CloseHandle(hProcess);
				}
			}
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
	LOG(INFO) << "kill processs [" << exeName << "] end";
	return true;
}


bool ProcessMonitor::start_self() {
	
	if (MonitorUtils::IsStopped()) {
		LOG(INFO) << "receive stop command, ignore start_self";
		return true;
	}
	HMODULE hModule = GetModuleHandleW(NULL);
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(hModule, path, MAX_PATH);
	wstring exeString = path;
	wifstream processfile(FileUtils::MonitorProcessFile());
	processfile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>));
	while (!processfile.eof()) {
		wstring processname;
		processfile >> processname;
		exeString += L" " + processname;
	}
	processfile.close();
	LOG(INFO) << MonitorUtils::ws2s(exeString).c_str();
	STARTUPINFO si = { sizeof(si) };
	
	return create_process(exeString);
}

wstring ProcessMonitor::get_name() {
	return m_cmd;
}

bool ProcessMonitor::start_process() {
	if (MonitorUtils::IsStopped())	{
		return false;
	}
	auto lock = m_lock.LockExclusive();
	if (m_state == ProcessState::Running || m_handle != nullptr) {
		return false;
	}

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	const auto cmd_copy_size = m_cmd.size() + 1;
	// Do nothing if allocation fails - exception just bubbles up
	// No cleanup needed
	auto cmd_copy = new wchar_t[cmd_copy_size];
	wcsncpy_s(cmd_copy, cmd_copy_size, m_cmd.c_str(), m_cmd.size());
	if (!CreateProcess(
		0,
		cmd_copy,
		0, 0, FALSE, CREATE_NO_WINDOW, 0, 0,
		&si,
		&pi)) {
		delete[] cmd_copy;
		err("Cannot create process");
	}
	delete[] cmd_copy;
	m_handle = pi.hProcess;

	if (!RegisterWaitForSingleObject(&m_process_exit_wait_handle, m_handle, ProcessExitedCallback, 
		this, INFINITE, WT_EXECUTEONLYONCE)) {
		err("Cannot subscribe for process termination");
	}

	m_state = ProcessState::Running;
	log("Process started");
	run_callbacks(m_on_proc_start_callbacks);
	return true;
}

void ProcessMonitor::log(const std::string& msg) {
	LOG(INFO) << msg.c_str();
}

void ProcessMonitor::err(const std::string& msg) {
	auto lock = m_lock.LockShared();
	LOG(ERROR) << msg.c_str();
	DebugBreak();
	throw Error(msg);
}

void ProcessMonitor::on_proc_start(const Callback& cb) {
	
	auto lock = m_lock.LockExclusive();
	m_on_proc_start_callbacks.push_back(cb);
}

void ProcessMonitor::on_proc_crash(const Callback& cb) {
	auto lock = m_lock.LockExclusive();
	m_on_proc_crash_callbacks.push_back(cb);
}

void ProcessMonitor::on_proc_normal_exit(const Callback& cb) {
	auto lock = m_lock.LockExclusive();
	m_on_proc_normal_exit_callbacks.push_back(cb);
}

void ProcessMonitor::on_proc_manually_stopped(const Callback& cb) {
	auto lock = m_lock.LockExclusive();
	m_on_proc_manually_stopped_callbacks.push_back(cb);
}

ProcessMonitor::~ProcessMonitor()
{
	if (m_handle != nullptr) {
		TerminateProcess(m_handle, 0);
	}
	// cleanup resources
	if (!m_process_exit_wait_handle) {
		UnregisterWait(m_process_exit_wait_handle);
		m_process_exit_wait_handle = nullptr;
	}

	if (m_handle != nullptr) {

		if (!CloseHandle(m_handle)) {
			DebugBreak();
		}
		m_handle = nullptr;
	}
	//ProcessMonitor::kill_process(MonitorUtils::ws2s(m_cmd));
}

bool ProcessMonitor::get_cmd_line_from_process() {
	// BASED ON http://stackoverflow.com/questions/7446887/get-command-line-string-of-64-bit-process-from-32-bit-process
	// and a few other internet sources
	// and painful 2-hour debugging session (because of the 32 vs 64 bit memory layout problems).
	DWORD err = 0;

	// determine if 64 or 32-bit processor
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);

	// determine if this process is running on WOW64
	BOOL wow;
	IsWow64Process(GetCurrentProcess(), &wow);

	// use WinDbg "dt ntdll!_PEB" command and search for ProcessParameters offset to find the truth out
	DWORD ProcessParametersOffset = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? 0x20 : 0x10;
	DWORD CommandLineOffset = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? 0x70 : 0x40;

	// read basic info to get ProcessParameters address, we only need the beginning of PEB
	DWORD pebSize = ProcessParametersOffset + 8;
	PBYTE peb = new BYTE[pebSize]{};

	// read basic info to get CommandLine address, we only need the beginning of ProcessParameters
	DWORD ppSize = CommandLineOffset + 16;
	PBYTE pp = new BYTE[ppSize]{};

	PWSTR cmdLine;

	if (wow) {
		// we're running as a 32-bit process in a 64-bit OS
		PROCESS_BASIC_INFORMATION_WOW64 pbi{};

		// get process information from 64-bit world
		auto query = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWow64QueryInformationProcess64");
		err = query(m_handle, 0, &pbi, sizeof(pbi), NULL);
		if (err != 0) {
			CloseHandle(m_handle);
			return false;
		}

		// read PEB from 64-bit address space
		auto read = (_NtWow64ReadVirtualMemory64)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWow64ReadVirtualMemory64");
		err = read(m_handle, pbi.PebBaseAddress, peb, pebSize, NULL);
		if (err != 0) {
			CloseHandle(m_handle);
			return false;
		}

		// read ProcessParameters from 64-bit address space
		auto parameters = (PBYTE*)*(LPVOID*)(peb + ProcessParametersOffset); // address in remote process adress space
		err = read(m_handle, parameters, pp, ppSize, NULL);
		if (err != 0) {
			CloseHandle(m_handle);
			return false;
		}

		// read CommandLine
		auto pCommandLine = (UNICODE_STRING_WOW64*)(pp + CommandLineOffset);
		cmdLine = (PWSTR) new char[pCommandLine->MaximumLength];
		err = read(m_handle, pCommandLine->Buffer, cmdLine, pCommandLine->MaximumLength, NULL);
		if (err != 0) {
			CloseHandle(m_handle);
			return false;
		}
	} else {
		// we're running as a 32-bit process in a 32-bit OS, or as a 64-bit process in a 64-bit OS
		PROCESS_BASIC_INFORMATION pbi{};

		// get process information
		auto query = (_NtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
		err = query(m_handle, 0, &pbi, sizeof(pbi), NULL);
		if (err != 0) {
			CloseHandle(m_handle);
			return false;
		}

		// read PEB
		if (!ReadProcessMemory(m_handle, pbi.PebBaseAddress, peb, pebSize, NULL)) {
			CloseHandle(m_handle);
			return false;
		}

		// read ProcessParameters
		auto parameters = (PBYTE*)*(LPVOID*)(peb + ProcessParametersOffset); // address in remote process adress space
		if (!ReadProcessMemory(m_handle, parameters, pp, ppSize, NULL))	{
			CloseHandle(m_handle);
			return false;
		}

		// read CommandLine
		auto pCommandLine = (UNICODE_STRING*)(pp + CommandLineOffset);
		cmdLine = (PWSTR) new char[pCommandLine->MaximumLength];
		if (!ReadProcessMemory(m_handle, pCommandLine->Buffer, cmdLine, pCommandLine->MaximumLength, NULL)) {
			CloseHandle(m_handle);
			return false;
		}
	}

	m_cmd = std::wstring(cmdLine);
	delete[] cmdLine, peb, pp;
	return true;
}

void ProcessMonitor::process_exited() {
	auto lock = m_lock.LockExclusive();
	// Don't restart if manually stopped
	if (m_state != ProcessState::Stopped) {
		m_state = ProcessState::Restarting;
		DWORD exit_code;
		GetExitCodeProcess(m_handle, &exit_code);
		if (exit_code != 0) {
			log("Process crashed (non-zero exit code)");
			run_callbacks(m_on_proc_crash_callbacks);
		} else {
			log("Process exited with 0 exit-code");
			run_callbacks(m_on_proc_normal_exit_callbacks);
		}
		m_handle = nullptr;
		lock.Unlock();
		start_process();
	}
}

void ProcessMonitor::run_callbacks(const Callbacks& callbacks) {
	// m_lock is assumed to be already locked
	for (const auto& cb : callbacks) {
		// First option:
		// run callbacks in their own threads
		// all exceptions from them are automatically handled
		// std::async(cb); 

		// Second option:
		// run callbacks in one thread
		// if one callback blocks - other won't be executed
		// if exceptions are thrown - program crashes.
		cb();

		// Third option:
		// run callbacks wrapped into try/catch in one thread
		// contradicts "fail early" philosophy
		// try {
		// cb();
		// } catch(...) {
		// 
		// }
	}
}

void ProcessMonitor::ProcessExitedCallback(void* lpParam, unsigned char) {
	// Forward call to instance member function.
	// pm is guaranteed to be a valid pointer 
	// because of the unsubscribing from this callback 
	// when instance is destructing
	auto pm = reinterpret_cast<ProcessMonitor*>(lpParam);
	pm->process_exited();
}

