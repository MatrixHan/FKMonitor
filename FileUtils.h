#pragma once
#include <string>
using namespace std;

class FileUtils {
public:
	static string WorkDir() ;
	static string DataDir() ;
	static string LogDir() ;
	static string FlagDir() ;
	static string StopFlagFile() ;
	static string MonitorProcessFile() ;
	static bool DirExist(const char * path);
	static bool FileExist(const char * path);
	static bool MkDir(const char * path);
	static bool NewFile(const char * path);
	static bool RemoveFile(const char * path);
	static bool ChDir(const char * path);
};