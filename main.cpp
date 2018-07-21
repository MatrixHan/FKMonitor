#include "processmonitor.h"
#include <iostream>
#include <cstdlib>
#include <memory>
#include "Init.h"
#include "easylogging++.h"
#include "FileUtils.h"
#include "SingleProcess.h"
#include "MonitorUtils.h"
#include "ParentMonitor.h"
#include <tchar.h>
using namespace std;



int _tmain(int argc, _TCHAR * argv[]) {
	if (argc >= 2 && wstring(argv[1]).substr(0, 4) == wstring(L"stop")) {
		MonitorUtils::Stop();
		exit(0);
	}
	
	SingleProcess * ProcessMaster = new SingleProcess(L"DNSMonitorMaster");
	if (!ProcessMaster->IsAnotherInstanceRunning()) {
		if (argc < 3) {
			printf("processlist is null\n");
			exit(1);
		}
		Init(true);
		MonitorUtils::ClearStop();
		ofstream ProcessFile(FileUtils::MonitorProcessFile(), ios_base::binary);
		for (int i = 1; i < argc; i++) {
			wstring ws(argv[i]);
			ProcessFile << MonitorUtils::ToUtf8(ws) << std::endl;
			// 第一个进程是需要监控的父进程，不杀
			if (i != 1) {
				ProcessMonitor::kill_process(argv[i]);
			}
		}
		ProcessFile.flush();
		ProcessFile.close();

		ParentMonitor::start_thread(argv[1]);

		vector<ProcessMonitor*> ProcessList;
		
		for (int i = 2; i < argc; i++) {
			ProcessMonitor * pm = new ProcessMonitor(argv[i]);
			pm->on_proc_start([pm] { LOG(INFO) << pm->get_name() << " Proc started";  });
			pm->on_proc_crash([pm] { LOG(INFO) << pm->get_name()  << " Proc crashed";  });
			pm->on_proc_normal_exit([pm] { LOG(INFO) << pm->get_name() << " Proc exited normally";  });
			pm->on_proc_manually_stopped([pm] { LOG(INFO) << pm->get_name() << " Proc manually stopped";  });
			ProcessList.push_back(pm);

			LOG(INFO) << "start process : " << argv[i] << " pid: " << pm->get_pid();
		}
		
		while (!MonitorUtils::IsStopped()) {
			Sleep(100);
			SingleProcess *p = new SingleProcess(L"DNSMonitorSlave");
			bool isRunning = p->IsAnotherInstanceRunning();
			delete p;

			if (!isRunning) {
				LOG(INFO) << "slave exit...";
				ProcessMonitor::start_self();
				LOG(INFO) << "slave start success...";
				Sleep(500);
			}
		}

		for (auto it = ProcessList.begin(); it != ProcessList.end(); ++it) {
			(*it)->stop_process(0);
		}
		// sleep 1000是为了等待stop命令后自动退出，如果1s内仍然没有退出将会被terminiate
		Sleep(1000);
		for (auto it = ProcessList.begin(); it != ProcessList.end(); ++it) {
			delete (*it);
		}
		LOG(INFO) << "master return gracefully";
	}
	else {
		// 释放占用的pipe
		delete ProcessMaster;
		SingleProcess ProcessSlave(L"DNSMonitorSlave");
		
		if (ProcessSlave.IsAnotherInstanceRunning()) {
			printf("master and slave is running\n");
			exit(1);
		}
		Init(false);
		while (!MonitorUtils::IsStopped()) {
			SingleProcess *p = new SingleProcess(L"DNSMonitorMaster");
			bool isRunning = p->IsAnotherInstanceRunning();
			delete p;
			if (!isRunning) {
				LOG(INFO) << "master exit...";
				ProcessMonitor::start_self();
				LOG(INFO) << "master start success...";
			}
			//else {
			//	LOG(INFO) << "master is running...";
			//}
			Sleep(100);
		}
		LOG(INFO) << "slave return gracefully";
	}
	el::Loggers::flushAll();
	return EXIT_SUCCESS;
}
