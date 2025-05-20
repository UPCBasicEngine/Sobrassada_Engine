#include "FileSystem.h"

#include "rapidjson/istreamwrapper.h"
#include "DetourAlloc.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <unistd.h>
#endif

namespace FileSystem
{
    unsigned int Load(const char* filePath, char** buffer, bool asBinary)
    {
        std::ifstream file(filePath, std::ios::in | std::ios::ate | (asBinary ? std::ios::binary : 0));

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
    //Needs special allocation and C-style loading
    unsigned int LoadForDetour(const char* filePath, char** buffer)
    {
        std::ifstream file(filePath, std::ios::in | std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            GLOG("Failed to open file: %s", filePath);
            return 0;
        }

        size_t size = file.tellg();
        file.seekg(0, std::ifstream::beg);

        *buffer = (char*)dtAlloc(size, DT_ALLOC_PERM);
        if (!*buffer)
        {
            file.close();
            GLOG("dtAlloc failed for navmesh data: %s", filePath);
            return 0;
        }

        if (file.read((char*)*buffer, size))
        {
            file.close();
            return static_cast<unsigned int>(size);
        }
        else
        {
            dtFree(*buffer); // free Detour memory
            *buffer = nullptr;
            file.close();
            GLOG("Failed to read navmesh data: %s", filePath);
            return 0;
        }
    }


    bool LoadJSON(const char* scenePath, rapidjson::Document& doc)
    {
        std::ifstream file(scenePath);
        if (!file.is_open())
        {
            GLOG("Failed to open file: %s", scenePath);
            return false;
        }

        rapidjson::IStreamWrapper sw(file);

        doc.ParseStream(sw);

        if (doc.HasParseError())
        {
            GLOG("Failed to parse scene JSON: %s", scenePath);
            return false;
        }

        file.close();
        return true;
    }

    unsigned int Save(const char* filePath, const void* buffer, unsigned int size, bool asBinary, bool append)
    {
        std::ofstream file(filePath, std::ios::out | (asBinary ? std::ios::binary : 0) | (append ? std::ios::app : 0));

        if (!file.is_open())
        {
            GLOG("Failed to open file: %s", filePath);
            return 0;
        }

        file.write(static_cast<const char*>(buffer), size);
        file.close();

        if (!file)
        {
            GLOG("Failed to write data to file: %s", filePath);
            return 0;
        }

        return size;
    }

    bool Copy(const char* sourceFilePath, const char* destinationFilePath)
    {
        if (!Exists(sourceFilePath))
        {
            GLOG("Source file does not exist: %s", sourceFilePath);
            return false;
        }

        std::error_code errorCode;
        std::filesystem::copy(
            sourceFilePath, destinationFilePath, std::filesystem::copy_options::update_existing, errorCode
        );

        if (errorCode)
        {
            GLOG("Failed to copy: %s", errorCode.message().c_str());
            return false;
        }

        return true;
    }

    void GetDrives(std::vector<std::string>& drives)
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
        for (const auto& entry : std::filesystem::directory_iterator(mountPoint))
        {
            if (std::filesystem::is_directory(entry))
            {
                drives.push_back(entry.path().string());
            }
        }
#endif
    }

    void GetAllInDirectory(const std::string& path, std::vector<std::string>& files)
    {
        files.clear();
        if (IsDirectory(path.c_str()))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                files.push_back(GetFileNameWithExtension(entry.path().string()));
            }
        }
        else
        {
            GLOG("%s is not a valid path", path);
        }
    }

    void GetFilesSorted(const std::string& currentPath, std::vector<std::string>& files)
    {
        // files & dir in the current directory
        GetAllInDirectory(currentPath, files);

        std::sort(
            files.begin(), files.end(),
            [&](const std::string& a, const std::string& b)
            {
                bool isDirA = FileSystem::IsDirectory((currentPath + DELIMITER + a).c_str());
                bool isDirB = FileSystem::IsDirectory((currentPath + DELIMITER + b).c_str());

                if (isDirA != isDirB) return isDirA > isDirB;
                return a < b;
            }
        );
    }

    void SplitAccumulatedPath(const std::string& path, std::vector<std::string>& accPaths)
    {
        accPaths.clear();
        std::string modifiedPath = path;
        std::replace(modifiedPath.begin(), modifiedPath.end(), '/', '\\');

        std::stringstream ss(modifiedPath);
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
                accPaths.push_back(accumulatedPath);
            }
        }
    }

    time_t GetLastModifiedTime(const std::string& path)
    {
        struct stat fileInfo;

        if (stat(path.c_str(), &fileInfo) != 0)
        {
            return 0;
        }

        return fileInfo.st_mtime;
    }

    void AddDelimiterIfNotPresent(std::string& path)
    {
        RemoveDelimiterIfPresent(path);
        path += DELIMITER;
    }

    void RemoveDelimiterIfPresent(std::string& path)
    {
        while (!path.empty() && path.back() == '\\' || path.back() == '/')
        {
            path.pop_back();
        }
    }
} // namespace FileSystem