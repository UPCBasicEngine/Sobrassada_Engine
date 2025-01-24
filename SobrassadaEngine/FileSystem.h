#pragma once

namespace FileSystem
{
	unsigned int Load(const char* filePath, char** buffer);
	unsigned int Save(const char* filePath, const void* buffer, unsigned int size, bool append = false);
	bool Copy(const char* sourceFilePath, const char* destinationFilePath);
	bool Delete(const char* filePath);
	bool CreateDirectory(const char* directoryPath);
	bool Exists(const char* filePath);
	bool IsDirectory(const char* directoryPath);
}
