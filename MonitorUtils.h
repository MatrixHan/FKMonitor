#pragma once
#include <codecvt>
#include <vector>
using namespace std;
class MonitorUtils
{
public:
	static wstring s2ws(const std::string& str);
	static string ws2s(const std::wstring& wstr);
	static string ToLower(const std::string & str);
	static string ToUpper(const std::string & str);
	static void ClearStop();
	static void Stop();
	static bool IsStopped();
	static std::string ToUtf8(const std::wstring& str);
};

