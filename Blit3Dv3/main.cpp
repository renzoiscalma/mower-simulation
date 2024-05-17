/*
Example program that shows how to display tile maps
*/
#include "Blit3D.h"
#include "TileMap.h"
#include "Robot.h"

Blit3D *blit3D = NULL;

//memory leak detection
#define CRTDBG_MAP_ALLOC
#ifdef _DEBUG
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif
#endif  // _DEBUG

#include <stdlib.h>
#include <crtdbg.h>
#include <iomanip>
#include <sstream>

//GLOBAL DATA
//sprite pointers
Sprite *tileSpriteSheet = NULL;
Sprite* robotSprite = NULL;

std::vector<Sprite *> tileSpriteList;

// font pointers
TileMap *tileMap = NULL;
AngelcodeFont *afont = NULL;

float elapsedTime = 0;
// time slice of 100th of a second
float timeSlice = 1.f / 100.f;

Robot* robot;

void Init()
{
	//Angelcode font
	afont = blit3D->MakeAngelcodeFontFromBinary32("Media\\Oswald_72.bin");

	//load the tilemap sprite
	tileSpriteSheet = blit3D->MakeSprite(0, 0, 480, 256, "Media\\BOF22_edited.png");

	//load the robot sprite
	robotSprite = blit3D->MakeSprite(0, 0, 512, 512, "Media\\robot.png");

	//load individual 16x16 tiles
	unsigned int x, y;
	for (y = 0; y < 256 / 16; ++y)
	{
		for (x = 0; x < 480 / 16; ++x)
		{
			tileSpriteList.push_back(blit3D->MakeSprite(x * 16, y * 16, 16, 16, "Media\\BOF22_edited.png"));
		}
	}

	tileMap = new TileMap("mapfile.dat");

	robot = new Robot(1, 1, robotSprite);

}

void DeInit(void)
{
	if (tileMap) delete tileMap;
}

void Update(double seconds)
{
	if (seconds < 0.15) elapsedTime += static_cast<float>(seconds);
	else elapsedTime += 0.15f;

	while (elapsedTime >= timeSlice)
	{
		// update loop. every 60th of a second this loop will run
		elapsedTime -= timeSlice;
		// old method of clearing tiles
		//robot->Update(timeSlice);
		// new method of clearing tiles
		robot->Update2(timeSlice);

	}
}

void Draw(void)
{
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);	//clear colour: r,g,b,a 	
	// wipe the drawing surface clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//draw the map
	tileMap->Draw(robot);
	// draw robot
	robot->Draw();


	// draw texts on scren
	std::string message;
	std::stringstream messageStream; // for strings with float formatting
	message = "Tiles to mow: " + std::to_string(tileMap->getTilesToMow());
	afont->BlitText(50, blit3D->screenHeight - 50, message);
	message = "Tiles mowed: " + std::to_string(tileMap->getTilesMowed());
	afont->BlitText(50, blit3D->screenHeight - 150, message);
	message = "Tile #: X: " + std::to_string((int)robot->getTileMapPosition().x)
		+ " Y: " + std::to_string((int)robot->getTileMapPosition().y);
	afont->BlitText(50, 100, message);
	messageStream << "Time: " << std::fixed << std::setprecision(4) << robot->getTimePassed() << " hrs";
	afont->BlitText(50, 200, messageStream.str());
	messageStream.str(""); // empty the stream
	messageStream << "Battery: " << std::fixed << std::setprecision(2) << robot->getBattery() << "%";
	afont->BlitText(blit3D->screenWidth - 500, 100, messageStream.str());
	messageStream.str("");
	messageStream << "Charges: " << std::to_string(robot->getRechargeCount());
	afont->BlitText(blit3D->screenWidth - 500, 200, messageStream.str());

	if (robot->getState() == RobotState::STOP && tileMap->getTilesToMow() != 0) {
		message = "Press space to start!";
		afont->BlitText(blit3D->screenWidth / 2 - 200, blit3D->screenHeight/2, message);
	}

	if (tileMap->getTilesToMow() == 0) {
		message = "ROBOT FINISHED MOWING THE AREA";
		afont->BlitText(blit3D->screenWidth / 2 - 500, blit3D->screenHeight / 2, message);
	}

}

//the key codes/actions/mods for DoInput are from GLFW: check its documentation for their values
void DoInput(int key, int scancode, int action, int mods)
{	
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		blit3D->Quit(); //start the shutdown sequence

	if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
	{
		robot->start();
	}

	// below code is for debugging 
	// long press arrow keys when you want to manually move the robot
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
	{
		robot->moveToDirection(RIGHT);
	}
	else if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
	{
		robot->start();

	}

	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		robot->moveToDirection(LEFT);
	}
	else if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
	{
		robot->start();
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		robot->moveToDirection(DOWN);
	}
	else if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
	{
		robot->start();
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		robot->moveToDirection(UP);
	}
	else if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
	{
		robot->start();
	}
}

// functions below are part of blit3D and code of darren, don't want to delete...

void DoCursor(double x, double y)
{
}

void DoMouseButton(int button, int action, int mods)
{
}

//called whenever the user resizes the window
void DoResize(int width, int height)
{
	blit3D->trueScreenWidth = blit3D->screenWidth = static_cast<float>(width);
	blit3D->trueScreenHeight = blit3D->screenHeight = static_cast<float>(height);
	blit3D->Reshape(blit3D->shader2d);
}

int main(int argc, char *argv[])
{
	//memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	blit3D = new Blit3D(Blit3DWindowModel::DECORATEDWINDOW, 1280, 768);
	//blit3D = new Blit3D(Blit3DWindowModel::FULLSCREEN, 920, 680);


	//set our callback funcs
	blit3D->SetInit(Init);
	blit3D->SetDeInit(DeInit);
	blit3D->SetUpdate(Update);
	blit3D->SetDraw(Draw);
	blit3D->SetDoInput(DoInput);
	blit3D->SetDoCursor(DoCursor);
	blit3D->SetDoMouseButton(DoMouseButton);
	blit3D->SetDoResize(DoResize);

	//Run() blocks until the window is closed
	blit3D->Run(Blit3DThreadModel::SINGLETHREADED);
	if (blit3D) delete blit3D;
}