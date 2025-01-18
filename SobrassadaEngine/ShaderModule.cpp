#include "ShaderModule.h"

#include "glew.h"

ShaderModule::ShaderModule()
{
}

ShaderModule::~ShaderModule()
{
}

unsigned int ShaderModule::GetProgram(const char* vertexPath, const char* fragmentPath)
{
	GLOG("Loading shaders")
	unsigned int program = 0;

	char* vertexShader = LoadShaderSource(vertexPath);
	char* fragmentShader = LoadShaderSource(fragmentPath);

	unsigned int vertexId = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fragmentId = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	program = CreateProgram(vertexId, fragmentId);

	free(vertexShader);
	free(fragmentShader);

	return program;
}

char* ShaderModule::LoadShaderSource(const char* shaderPath)
{
	GLOG("Reading shader: %s", shaderPath)
	char* data = nullptr;
	FILE* file = nullptr;

	fopen_s(&file, shaderPath, "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		data = (char*)malloc(size + 1);
		fseek(file, 0, SEEK_SET);
		fread(data, 1, size, file);
		data[size] = 0;
		fclose(file);
	}

	return data;
}

unsigned int ShaderModule::CompileShader(unsigned int shaderType, const char* source)
{ 
	GLOG("Compiling %s", GL_VERTEX_SHADER == shaderType ? "vertex shader" : "fragment shader")
	unsigned shaderId = glCreateShader(shaderType);
	glShaderSource(shaderId, 1, &source, 0);
	glCompileShader(shaderId);

	int compileSuccess = GL_FALSE;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileSuccess);

	if (compileSuccess == GL_FALSE)
	{
		GLOG("Error compiling %s", GL_VERTEX_SHADER == shaderType ? "vertex shader" : "fragment shader")
			int logLenght = 0;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLenght);
		if (logLenght > 0)
		{
			int written = 0;
			char* info = (char*)malloc(logLenght);
			glGetShaderInfoLog(shaderId, logLenght, &written, info);
			GLOG("Log Info: %s", info);
			free(info);
		}
	}

	return shaderId;
}

unsigned int ShaderModule::CreateProgram(unsigned int vertexShader, unsigned fragmentShader)
{
	GLOG("Creating shader program")
	unsigned programId = glCreateProgram();
	glAttachShader(programId, vertexShader);
	glAttachShader(programId, fragmentShader);
	glLinkProgram(programId);

	int compileSuccess = GL_FALSE;
	glGetProgramiv(programId, GL_LINK_STATUS, &compileSuccess);

	if (compileSuccess == GL_FALSE)
	{
		GLOG("Error creating shader program")
			int logLenght = 0;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLenght);

		if (logLenght > 0)
		{
			int written = 0;
			char* info = (char*)malloc(logLenght);
			glGetProgramInfoLog(programId, logLenght, &written, info);
			GLOG("Program Log Info: %s", info);
			free(info);
		}
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programId;
}

void ShaderModule::DeleteProgram(unsigned int programId)
{
	glDeleteProgram(programId);
}
