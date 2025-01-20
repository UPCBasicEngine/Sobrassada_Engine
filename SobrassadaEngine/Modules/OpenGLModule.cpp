#include "OpenGLModule.h"

#include "Application.h"
#include "WindowModule.h"
#include "Framebuffer.h"

#include "glew.h"
#include "Windows.h"

OpenGLModule::OpenGLModule()
{
	context = nullptr;
}

OpenGLModule::~OpenGLModule()
{
}

bool OpenGLModule::Init()
{
	GLOG("Creating Renderer context");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	context = SDL_GL_CreateContext(App->GetWindowModule()->window);
	GLenum err = glewInit();

	GLOG("Using Glew %s", glewGetString(GLEW_VERSION));

	GLOG("Vendor: %s", glGetString(GL_VENDOR));
	GLOG("Renderer: %s", glGetString(GL_RENDERER));
	GLOG("OpenGL version supported %s", glGetString(GL_VERSION));
	GLOG("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glEnable(GL_DEPTH_TEST); // Enable depth test
	glEnable(GL_CULL_FACE); // Enable cull backward faces
	glFrontFace(GL_CCW);	// Enable conter clock wise backward faces

	framebuffer = new Framebuffer(App->GetWindowModule()->GetWidth(), App->GetWindowModule()->GetHeight(), true);

	return true;
}

update_status OpenGLModule::PreUpdate(float deltaTime)
{
	int CurrentWidth = 0;
	int CurrentHeight = 0;
	SDL_GetWindowSize(App->GetWindowModule()->window, &CurrentWidth, &CurrentHeight);
	
	framebuffer->Bind();
	
	if (CurrentWidth && CurrentHeight) {
		glViewport(0, 0, framebuffer->GetTextureWidth(), framebuffer->GetTextureHeight());

		glClearColor(clearColorRed, clearColorGreen, clearColorBlue, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return UPDATE_CONTINUE;
}

update_status OpenGLModule::Update(float deltaTime)
{
	return UPDATE_CONTINUE;
}

update_status OpenGLModule::PostUpdate(float deltaTime)
{
	framebuffer->CheckResize();

	SDL_GL_SwapWindow(App->GetWindowModule()->window);

	return UPDATE_CONTINUE;
}

bool OpenGLModule::ShutDown()
{
	GLOG("Destroying renderer");

	SDL_GL_DeleteContext(App->GetWindowModule()->window);
	delete framebuffer;
	return true;
}

void OpenGLModule::WindowResized(unsigned width, unsigned height)
{
}

void OpenGLModule::SetDepthTest(bool enable)
{
	if (enable) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);
}

void OpenGLModule::SetFaceCull(bool enable)
{
	if (enable) glEnable(GL_CULL_FACE);
	else glDisable(GL_CULL_FACE);
}

void OpenGLModule::SetFrontFaceMode(int mode)
{
	glFrontFace(mode);
}

void OpenGLModule::SetClearRed(float newValue)
{
	clearColorRed = newValue;
}

void OpenGLModule::SetClearGreen(float newValue)
{
	clearColorGreen = newValue;
}

void OpenGLModule::SetClearBlue(float newValue)
{
	clearColorBlue = newValue;
}
