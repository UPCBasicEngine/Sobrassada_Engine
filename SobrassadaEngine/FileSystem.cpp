#include "FileSystem.h"

#include "Globals.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <sstream>
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <unistd.h>
#endif

namespace FileSystem
{
	void GetDrives(std::vector<std::string>& drives)
	{
		drives.clear();
#if defined(_WIN32) || defined(_WIN64)
		DWORD driveMask = GetLogicalDrives();
		if (driveMask == 0) {
			GLOG("Error getting logical drives: %s", GetLastError());
		}
		else {
			for (char drive = 'A'; drive <= 'Z'; ++drive) {
				if (driveMask & (1 << (drive - 'A'))) {
					std::string drivePath = std::string(1, drive) + ":";
					drives.push_back(drivePath);
				}
			}
		}
#elif defined(__linux__) || defined(__APPLE__)
		std::string mountPoint = "/mnt";
		for (const auto& entry : std::filesystem::directory_iterator(mountPoint))
		{
			if (std::filesystem::is_directory(entry))
			{
				drives.push_back(entry.path().string());
			}
		}
#endif
	}

	void GetAllInDirectory(const std::string& path, std::vector<std::string>& files) {
		files.clear();
		if (IsDirectory(path.c_str())) {
			for (const auto& entry : std::filesystem::directory_iterator(path)) {
				files.push_back(GetFileNameWithExtension(entry.path().string()));
			}
		}
		else {
			GLOG("%s is not a valid path", path);
		}
	}

	void SplitAccumulatedPath(const std::string& path, std::vector<std::string>& parts)
	{
		parts.clear();
		std::stringstream ss(path);
		std::string part;
		std::string accumulatedPath;

		while (std::getline(ss, part, DELIMITER)) {
			if (!part.empty()) {
				if (!accumulatedPath.empty()) {
					accumulatedPath += DELIMITER;
				}
				accumulatedPath += part;
				parts.push_back(accumulatedPath);
			}
		}
	}
}