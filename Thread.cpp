#include "Thread.h"
#include <vector>
#include <thread>
#include <string>
#include "easylogging++.h"
#include <locale>
#include <codecvt>

using namespace std;

static wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

static string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}


static bool IsStop() {
	return false;
}
static vector<thread *> all_thread;

static void StartProcess(wstring processName) {


}
static void WaitStop() {

}
void Thread::MonitorProcess(int argc, char * argv[]){
	if (argc <= 1) {
		LOG(ERROR) << "can not find process to start";
		return;
	}
	for (int i = 1; i < argc; i++) {
		::StartProcess(s2ws(argv[i]));
	}
}
void Thread::WaitStop(){
	::WaitStop();
}
void Thread::MonitorSelf(){
}

