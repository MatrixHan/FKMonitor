#include "MonitorUtils.h"
#include "FileUtils.h"
#include <cctype>
#include <algorithm>
#include <windows.h>

wstring MonitorUtils::s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

string MonitorUtils::ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}
void MonitorUtils::ClearStop() {
	string stopFile = FileUtils::StopFlagFile();
	FileUtils::RemoveFile(stopFile.c_str());
}
void MonitorUtils::Stop() {
	string stopFile = FileUtils::StopFlagFile();
	FileUtils::NewFile(stopFile.c_str());
}
bool MonitorUtils::IsStopped() {
	string stopFile = FileUtils::StopFlagFile();
	if (FileUtils::FileExist(stopFile.c_str())) {
		return true;
	}
	return false;
}

string MonitorUtils::ToLower(const std::string & str) {
	string s = str;
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::tolower(c); } // correct
	);
	return s;
}
string MonitorUtils::ToUpper(const std::string & str) {
	string s = str;
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::toupper(c); } // correct
	);
	return s;
}

static std::string to_utf8(const wchar_t* buffer, int len)
{
	int nChars = ::WideCharToMultiByte(
		CP_UTF8,
		0,
		buffer,
		len,
		NULL,
		0,
		NULL,
		NULL);
	if (nChars == 0) return "";

	string newbuffer;
	newbuffer.resize(nChars);
	::WideCharToMultiByte(
		CP_UTF8,
		0,
		buffer,
		len,
		const_cast<char*>(newbuffer.c_str()),
		nChars,
		NULL,
		NULL);

	return newbuffer;
}

std::string MonitorUtils::ToUtf8(const std::wstring& str)
{
	return to_utf8(str.c_str(), (int)str.size());
}