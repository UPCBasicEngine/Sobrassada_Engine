#include "Application.h"
#include "UPCTimer.h"
#include "Globals.h"

#include "SDL.h"
#pragma comment( lib, "Libs/SDL/lib/SDL2.lib" )
#pragma comment( lib, "Libs/SDL/lib/SDL2main.lib" )

#include <iostream>
#include <memory>


enum MainState
{
	MAIN_CREATION,
	MAIN_START,
	MAIN_UPDATE,
	MAIN_FINISH,
	MAIN_EXIT
};

Application* App = NULL;
std::vector<char*>* Logs = NULL;

int main(int argc, char** argv)
{
	std::unique_ptr<UPCTimer> timer = std::make_unique<UPCTimer>();

	int mainReturn = EXIT_SUCCESS;
	MainState mainState = MAIN_CREATION;

	Logs = new std::vector<char*>();

	while (mainState != MAIN_EXIT)
	{
		switch (mainState)
		{
		case MAIN_CREATION:

			GLOG("----- Application Creation -----");
			App = new Application();
			mainState = MAIN_START;
			break;

		case MAIN_START:

			GLOG("----- Application Init -----");
			if (App->Init() == false)
			{
				GLOG("----- Application Init exits with error -----");
				mainState = MAIN_EXIT;
			}
			else
			{
				mainState = MAIN_UPDATE;
				GLOG("----- Application Update -----");
			}

			break;

		case MAIN_UPDATE:
		{
			float deltaTime = timer->Tick();
			int update_return = App->Update(deltaTime);

			if (update_return == UPDATE_ERROR)
			{
				GLOG("----- Application Update exits with error -----");
				mainState = MAIN_EXIT;
			}

			if (update_return == UPDATE_STOP)
				mainState = MAIN_FINISH;
		}
		break;

		case MAIN_FINISH:

			GLOG("----- Application CleanUp -----");
			if (App->CleanUp() == false)
			{
				GLOG("----- Application CleanUp exits with error -----");
			}
			else
				mainReturn = EXIT_SUCCESS;

			mainState = MAIN_EXIT;

			break;

		}
	}

	delete App;

	// Free memory from log*
	for (auto log : *Logs)
	{
		free(log);
	}
	Logs->clear();
	delete Logs;

	std::cout << "Hello world \n";

	return mainReturn;
}