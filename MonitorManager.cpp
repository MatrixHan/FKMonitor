#include "MonitorManager.h"
#include "Thread.h"

bool MonitorManager::IsMasterRunning(){

}
bool MonitorManager::IsSlaveRunning(){

}

bool SingleInstance(wstring s) {

}
void MonitorManager::Master(int argc, char * argv[]){
	if (MonitorManager::IsMasterRunning()) {
		return;
	}
	if (!SingleInstance(L"dnsmonitormaster")) {
		return;
	}
	Thread::MonitorProcess(argc, argv);
	Thread::MonitorSelf();
	Thread::WaitStop();
}
void MonitorManager::Slave(int argc, char * argv[]){
	if (MonitorManager::IsSlaveRunning()) {
		return;
	}
	if (!SingleInstance(L"dnsmonitorslave")) {
		return;
	}
	Thread::MonitorSelf();
	Thread::WaitStop();
}
void MonitorManager::Stop(){

}

