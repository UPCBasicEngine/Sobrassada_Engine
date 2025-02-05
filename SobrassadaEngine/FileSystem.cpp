#include "FileSystem.h"

#include "Globals.h"

#include <fstream>
#include <filesystem>
#include <string>

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
			GLOG("Failed to read data from file: %s", filePath);
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

		if (!file)
		{
			GLOG("Failed to write data to file: %s", filePath);
			return 0;
		}

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

	bool CreateDirectories(const char* directoryPath)
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

	std::string GetFilePath(const std::string& filePath) {
		return std::filesystem::path(filePath).parent_path().string() + "\\";
	}

	std::string GetFileNameWithExtension(const std::string& filePath) {
		return std::filesystem::path(filePath).filename().string();
	}

	std::string GetFileNameWithoutExtension(const std::string& filePath) {
		std::filesystem::path path(filePath);
		return path.stem().string();
	}

	std::string GetFileExtension(const std::string& filePath) {
		std::filesystem::path path(filePath);
		return path.extension().string();
	}
}