#pragma once
#define NOMINMAX
#include <stdio.h>
#include <vector>
#include <windows.h>
#include "Algorithm/Random/LCG.h"

extern std::vector<char *> *Logs;

#define GLOG(format, ...) glog(__FILE__, __LINE__, format, __VA_ARGS__);

void glog(const char file[], int line, const char *format, ...);

enum update_status
{
    UPDATE_CONTINUE = 1,
    UPDATE_STOP,
    UPDATE_ERROR
};

// Deletes an array of buffers
#define RELEASE_ARRAY(x)                                                                                               \
    {                                                                                                                  \
        if (x != nullptr)                                                                                              \
        {                                                                                                              \
            delete[] x;                                                                                                \
            x = nullptr;                                                                                               \
        }                                                                                                              \
    }

// Configuration -----------
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define FULLSCREEN    false
#define VSYNC         true
#define TITLE         "Sobrassada Engine"
#define HFOV          90

#ifdef _WIN32
#define DELIMITER '\\'
#else
#define DELIMITER '/'
#endif

#define DEFAULT_GL_CLEAR_COLOR_RED   0.5f
#define DEFAULT_GL_CLEAR_COLOR_GREEN 0.5f
#define DEFAULT_GL_CLEAR_COLOR_BLUE  0.5f

#define DEFAULT_CAMERA_MOVEMENT_SCALE_FACTOR 1.f;
#define DEFAULT_CAMERA_MOVEMENT_SPEED        7.5f;
#define DEFAULT_CAMERA_MOUSE_SENSITIVITY     0.5f;
#define DEFAULT_CAMERA_ZOOM_SENSITIVITY      5.f;

#define ASSETS_PATH     "Assets/"
#define ANIMATIONS_PATH "Library/Animations/"
#define AUDIO_PATH      "Library/Audio/"
#define BONES_PATH      "Library/Bones/"
#define MESHES_PATH     "Library/Meshes/"
#define TEXTURES_PATH   "Library/Textures/"
#define MATERIALS_PATH  "Library/Materials/"
#define SCENES_PATH     "Library/Scenes/"

#define ASSET_EXTENSION    ".gltf"
#define FILE_EXTENSION     ".sobrassada"
#define TEXTURE_EXTENSION  ".dds"
#define MATERIAL_EXTENSION ".mat"
#define SCENE_EXTENSION    ".scene"

constexpr float PI = 3.14159265359f;

#define UID uint64_t

inline UID GenerateUID()
{
    LCG rng;
    rng.IntFast();

    uint64_t uid = static_cast<uint64_t>(rng.IntFast()) << 32 | rng.IntFast(); // Combine two 32-bit values

    return uid;
}