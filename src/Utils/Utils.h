#pragma once

#include <string>
#include <iostream>


#ifdef OS_WINDOWS
#include <Windows.h>


inline std::string GetExecutableOsPath()
{
	char Path[_MAX_PATH];
	if (!GetModuleFileName(NULL, Path, MAX_PATH))
	{
		throw std::runtime_error("Ошибка получения исполняемого пути для OS Windows. Работа программы не возможна");
	}
	std::string fullpath(Path);
	size_t pos = fullpath.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : fullpath.substr(0, pos);
}
#endif

#ifdef OS_FREEBSD

#include <sys/types.h>
#include <sys/sysctl.h>
#include <libgen.h>

inline std::string GetExecutableOsPath()
{
	int mib[4];
	char buf[1204];
	size_t len = sizeof(buf);
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	if(sysctl(mib, 4, buf, &len, NULL, 0) == -1)
	{
		std::cout << "Error get executable path on freebsd";
	}
	return dirname(buf);
}


#endif
