#include "FileSystem.h"

#include "Globals.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <unistd.h>
#endif

namespace FileSystem
{
    unsigned int Load(const char *filePath, char **buffer)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);

        if (!file.is_open())
        {
            GLOG("Failed to open file: %s", filePath);
            return 0;
        }

        size_t size = file.tellg();
        file.seekg(0, std::ifstream::beg);

        *buffer = new char[size];
        if (file.read(*buffer, size))
        {
            file.close();
            return static_cast<unsigned int>(size);
        }
        else
        {
            delete[] *buffer;
            buffer = nullptr;
            file.close();
            GLOG("Failed to read data from file: %s", filePath);
            return 0;
        }
    }

    unsigned int Save(const char *filePath, const void *buffer, unsigned int size, bool append)
    {
        std::ofstream file(filePath, std::ios::binary | (append ? std::ios::app : 0));

        if (!file.is_open())
        {
            GLOG("Failed to open file: %s", filePath);
            return 0;
        }

        file.write(static_cast<const char *>(buffer), size);
        file.close();

        if (!file)
        {
            GLOG("Failed to write data to file: %s", filePath);
            return 0;
        }

        return size;
    }

    bool Copy(const char *sourceFilePath, const char *destinationFilePath)
    {
        if (!Exists(sourceFilePath))
        {
            GLOG("Source file does not exist: %s", sourceFilePath);
            return false;
        }

        std::error_code errorCode;
        std::filesystem::copy(
            sourceFilePath, destinationFilePath, std::filesystem::copy_options::overwrite_existing, errorCode
        );

        if (errorCode)
        {
            GLOG("Failed to copy: %s", errorCode.message().c_str());
            return false;
        }

        return true;
    }

    void GetDrives(std::vector<std::string> &drives)
    {
        drives.clear();
#if defined(_WIN32) || defined(_WIN64)
        DWORD driveMask = GetLogicalDrives();
        if (driveMask == 0)
        {
            GLOG("Error getting logical drives: %s", GetLastError());
        }
        else
        {
            for (char drive = 'A'; drive <= 'Z'; ++drive)
            {
                if (driveMask & (1 << (drive - 'A')))
                {
                    std::string drivePath = std::string(1, drive) + ":";
                    drives.push_back(drivePath);
                }
            }
        }
#elif defined(__linux__) || defined(__APPLE__)
        std::string mountPoint = "/mnt";
        for (const auto &entry : std::filesystem::directory_iterator(mountPoint))
        {
            if (std::filesystem::is_directory(entry))
            {
                drives.push_back(entry.path().string());
            }
        }
#endif
    }

    void GetAllInDirectory(const std::string &path, std::vector<std::string> &files)
    {
        files.clear();
        if (IsDirectory(path.c_str()))
        {
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                files.push_back(GetFileNameWithExtension(entry.path().string()));
            }
        }
        else
        {
            GLOG("%s is not a valid path", path);
        }
    }

    void SplitAccumulatedPath(const std::string &path, std::vector<std::string> &parts)
    {
        parts.clear();
        std::stringstream ss(path);
        std::string part;
        std::string accumulatedPath;

        while (std::getline(ss, part, DELIMITER))
        {
            if (!part.empty())
            {
                if (!accumulatedPath.empty())
                {
                    accumulatedPath += DELIMITER;
                }
                accumulatedPath += part;
                parts.push_back(accumulatedPath);
            }
        }
    }
} // namespace FileSystem