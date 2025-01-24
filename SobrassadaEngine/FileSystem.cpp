#include "FileSystem.h"

#include <fstream>
#include <filesystem>
#include <Utils/Globals.h> // should not be needed

namespace FileSystem
{
	unsigned int Load(const char* filePath, char** buffer)
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
		else {
			delete[] *buffer;
			buffer = nullptr;
			file.close();
			return 0;
		}
	}

	unsigned int Save(const char* filePath, const void* buffer, unsigned int size, bool append)
	{
		std::ofstream file(filePath, std::ios::binary | (append ? std::ios::app : 0));

		if (!file.is_open())
		{
			GLOG("Failed to open file: %s", filePath);
			return 0;
		}

		file.write(static_cast<const char*>(buffer), size);
		file.close();
		return size;
	}

	bool Copy(const char* sourceFilePath, const char* destinationFilePath)
	{
		try
		{
			std::filesystem::copy(sourceFilePath, destinationFilePath, std::filesystem::copy_options::overwrite_existing);
			return true;
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			GLOG("Failed to copy: %s", e.what());
			return false;
		}
	}

	bool Delete(const char* filePath)
	{
		return std::filesystem::remove(filePath);
	}

	bool CreateDirectory(const char* directoryPath)
	{
		return std::filesystem::create_directories(directoryPath);
	}

	bool Exists(const char* filePath)
	{
		return std::filesystem::exists(filePath);
	}

	bool IsDirectory(const char* directoryPath)
	{
		return std::filesystem::is_directory(directoryPath);
	}
}