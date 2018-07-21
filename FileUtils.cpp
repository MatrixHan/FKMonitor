#include "FileUtils.h"
#include <direct.h>
#include <stdio.h>

string FileUtils::WorkDir() {
	char* buf = nullptr;
	size_t sz = 0;
	if (_dupenv_s(&buf, &sz, "TEMP") == 0 && buf != nullptr)
	{
		string dir = buf;
		free(buf);
		dir += "\\dnsmonitor\\";
		return dir;
	}
	return "temp";
}
string FileUtils::DataDir() { 
	return WorkDir()+"data\\"; 
}
string FileUtils::LogDir() { 
	return WorkDir()+"log\\"; 
}
string FileUtils::FlagDir() {
	return WorkDir() + "flag\\";
}
string FileUtils::StopFlagFile() {
	return FlagDir() + "stop.flag";
}
string FileUtils::MonitorProcessFile() {
	return FlagDir() + "process_name.list";
}
bool FileUtils::DirExist(const char * path)
{
	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
		return false;
}

bool FileUtils::MkDir(const char * path) {
	if (FileUtils::DirExist(path)) {
		return true;
	}
	return _mkdir(path) == 0;
}

bool FileUtils::ChDir(const char * path) {
	if (!FileUtils::DirExist(path)) {
		return false;
	}
	return _chdir(path) == 0;
}
bool FileUtils::FileExist(const char * path)
{
	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	else if (info.st_mode & S_IFMT)
		return true;
	else
		return false;
}
bool FileUtils::NewFile(const char * path)
{
	if (FileExist(path)) {
		return true;
	}
	FILE * fp = nullptr;
	fopen_s(&fp, path, "a");
	if (!fp) {
		return false;
	}
	fclose(fp);
	return true;
}
bool FileUtils::RemoveFile(const char * path)
{
	if (!FileExist(path)) {
		return true;
	}
	return remove(path) != 0;
}