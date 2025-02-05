#pragma once

#include <string>

namespace FileSystem
{
	unsigned int Load(const char* filePath, char** buffer);
	unsigned int Save(const char* filePath, const void* buffer, unsigned int size, bool append = false);
	bool Copy(const char* sourceFilePath, const char* destinationFilePath);
	bool Delete(const char* filePath);
	bool CreateDirectories(const char* directoryPath);
	bool Exists(const char* filePath);
	bool IsDirectory(const char* directoryPath);
	std::string GetFilePath(const std::string& filePath);
	std::string GetFileNameWithExtension(const std::string& filePath);
	std::string GetFileNameWithoutExtension(const std::string& filePath);
	std::string GetFileExtension(const std::string& filePath);
	void ListFilesInDirectory(const std::string& path);
}