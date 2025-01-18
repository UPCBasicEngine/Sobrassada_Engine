#pragma once
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <vector>

extern std::vector<char*>* Logs;

#define GLOG(format, ...) glog(__FILE__, __LINE__, format, __VA_ARGS__);

void glog(const char file[], int line, const char* format, ...);

enum update_status
{
	UPDATE_CONTINUE = 1,
	UPDATE_STOP,
	UPDATE_ERROR
};

// Deletes an array of buffers
#define RELEASE_ARRAY( x ) \
	{\
       if( x != nullptr )\
       {\
           delete[] x;\
	       x = nullptr;\
		 }\
	 }


// Configuration -----------
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define FULLSCREEN false
#define VSYNC true
#define TITLE "Super Awesome Engine"
#define HFOV 90

#define DEFAULT_GL_CLEAR_COLOR_RED 0.5f
#define DEFAULT_GL_CLEAR_COLOR_GREEN 0.5f
#define DEFAULT_GL_CLEAR_COLOR_BLUE 0.5f

#define DEFAULT_CAMERA_MOVEMENT_SCALE_FACTOR 1.f;
#define DEFAULT_CAMERA_MOVMENT_SPEED 7.5f;
#define DEFAULT_CAMERA_MOUSE_SENSITIVITY 0.5f;
#define DEFAUL_CAMERA_ZOOM_SENSITIVITY 5.f;