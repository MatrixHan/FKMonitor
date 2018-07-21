#include "Init.h"
#include "easylogging++.h"
#include "FileUtils.h"

#include "Err.h"

static void HideWindow() {

	//HWND myConsole = GetConsoleWindow();
	//ShowWindow(myConsole, 0);
	::FreeConsole();
	return;
	/*
	HWND window;
	AllocConsole();
	window = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(window, 0);
	return ;
	*/
}

static bool AdjustPrivilege()
{
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
		{
			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		}
		CloseHandle(hToken);
		return true;
	} 
	return false;
}

static ERR UAV(){
	if (AdjustPrivilege() == false) {
		return ERR("AdjustPrivilege failed");
	}
	return OK;
}

static ERR WorkingDir(){
	string dir = FileUtils::WorkDir();
	bool ret = false;
	ret = FileUtils::MkDir(dir.c_str());
	if (!ret) {
		return ERR("mk work dir error");
	}
	ret = FileUtils::ChDir(dir.c_str());
	if (!ret) {
		return ERR("chdir work dir error");
	}
	dir = FileUtils::LogDir();
	ret = FileUtils::MkDir(dir.c_str());
	if (!ret) {
		return ERR("mk log dir error");
	}
	dir = FileUtils::DataDir();
	ret = FileUtils::MkDir(dir.c_str());
	if (!ret) {
		return ERR("mk data dir error");
	}
	dir = FileUtils::FlagDir();
	ret = FileUtils::MkDir(dir.c_str());
	if (!ret) {
		return ERR("mk data dir error");
	}
	return OK;
}

static ERR LogMaster() {

	string config = R"(
* GLOBAL:
       FORMAT = [%datetime] [%fbase:%line %thread %level]: %msg
	   FILENAME = "log/dnsmonitormaster.log"
	   ENABLED = true
	   TO_FILE = true
	   TO_STANDARD_OUTPUT = false
	   SUBSECOND_PRECISION = 6
	   PERFORMANCE_TRACKING = true
	   MAX_LOG_FILE_SIZE = 10485760 ## 10MB - Comment starts with two hashes(##)
	   LOG_FLUSH_THRESHOLD = 1 ## Flush after every 100 logs
)";

	el::Configurations c;
	c.parseFromText(config);
	el::Loggers::reconfigureLogger("default", c);
	// Actually reconfigure all loggers instead
	el::Loggers::reconfigureAllLoggers(c);
	el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
	return OK;
}
static ERR LogSlave() {

	string config = R"(
* GLOBAL:
       FORMAT = [%datetime] [%fbase:%line %thread %level]: %msg
	   FILENAME = "log/dnsmonitorslave.log"
	   ENABLED = true
	   TO_FILE = true
	   TO_STANDARD_OUTPUT = false
	   SUBSECOND_PRECISION = 6
	   PERFORMANCE_TRACKING = true
	   MAX_LOG_FILE_SIZE = 10485760 ## 10MB - Comment starts with two hashes(##)
	   LOG_FLUSH_THRESHOLD = 1 ## Flush after every 100 logs
)";

	el::Configurations c;
	c.parseFromText(config);
	el::Loggers::reconfigureLogger("default", c);
	// Actually reconfigure all loggers instead
	el::Loggers::reconfigureAllLoggers(c);
	el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
	return OK;
}

INITIALIZE_EASYLOGGINGPP;
Init::Init(bool isMaster) {
	HideWindow();
	ERR err;
	err = UAV();
	if (!err.IsOK()) {
		fprintf(stderr,"UAV init error %s\n", err.String().c_str());
		exit(1);
	}

	err = WorkingDir();
	if (!err.IsOK()) {
		fprintf(stderr,"WorkingDir init error %s\n", err.String().c_str());
		exit(1);
	}
	if (isMaster){
		err = LogMaster();
	}
	else {
		err = LogSlave();
	}
	if (!err.IsOK()) {
		fprintf(stderr,"Log init error %s\n", err.String().c_str());
		exit(1);
	}
	
	LOG(INFO) << "init ok"; 
}