//Copyright (c) 2020 Jacob Large
//This program is a fully external tool for assisting in bunnyhopping in all games based on the Orange Box engine.

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include "CProcess.h"
#include <time.h>

//Struct to store offsets, gamewindow, and game name
struct offset
{
	offset();
	DWORD ON_GROUND;
	DWORD SPECTATING;
	DWORD OVERLAY;
	DWORD WATER;
	HWND game;
	char *gameName;
};

//Initializer function to set all members to NULL
offset::offset()
{
	ON_GROUND = NULL;
	SPECTATING = NULL;
	OVERLAY = NULL;
	game = NULL;
}

//Prototype functions
bool isSpectating();
bool onGround();
bool overlayOpen();
void bhopLoop(const offset&);
void sendSpace(const offset &addrs);
void findGame(offset &addrs);

//Global variables
int grounded = -1;
int overlay = -1;
int spectate = -1;
int water = -1;
int grmax = 0;
double timestart;
CProcess worker;

int main()
{
	SetConsoleTitle("svHop External Sven Co-op Bunnyhop Tool");
	while (1){
		
		std::cout << "svHop External Sven Co-op Bunnyhop Tool by Lagunka" << std::endl;
		std::cout << "Waiting for Sven Co-op..." << std::endl;
		worker.Initialize();//Searches for svencoop.exe
		std::cout << "svencoop.exe found. Waiting for game window..." << std::endl;
		offset addrs;//Creates a struct object for storing offsets and game name
		findGame(addrs);//Searches for a source game window
		bhopLoop(addrs);//Main loop for bhopping
		Sleep(2000);
		worker.dwEngine = 0;
		worker.dwOverlay = 0;
		delete[] addrs.gameName;//Free the memory allocated for the dynamic array
	}
	return 0;
}

//Reads the process memory to see if the client is on ground
//Because the values can start becoming other than 0 and 1 (3 and 4 for example) some fixes were applied
bool onGround(offset addrs)
{
	if (grounded > grmax)//Stores the max value of the address
		grmax = grounded;
	while (grmax - 1 > grounded)//If max value is more than 1 more of the min value, it lowers it to be 1 more again
		grmax -= 1;
	ReadProcessMemory(worker.hProcess, (LPVOID)(worker.dwEngine + addrs.ON_GROUND), &grounded, sizeof(grounded), 0);//reads the process memory and stores the value to grounded
	if (grounded == grmax)
		timestart = time(NULL);
	else
		timestart = 0.0;
	return grounded == grmax;//returns true if grounded is at it's max, false if not.
}

//Function to check if the client is in the water
bool isInWater(offset addrs)
{
	ReadProcessMemory(worker.hProcess, (LPVOID)(worker.dwEngine + addrs.WATER), &water, sizeof(water), 0);
	return water > 0;
}
//Function to check if the client is in spectate (only applies to CS:S and DOD:S)
bool isSpectating(offset addrs)
{
	ReadProcessMemory(worker.hProcess, (LPVOID)(worker.dwEngine + addrs.SPECTATING), &spectate, sizeof(spectate), 0);
	return spectate == 1;
}

//Function to check if the steam overlay is open so spaces don't get sent while typing
bool overlayOpen(offset addrs)
{
	ReadProcessMemory(worker.hProcess, (LPVOID)(worker.dwOverlay + addrs.OVERLAY), &overlay, sizeof(overlay), 0);
	return overlay == 1;
}

//This function loops until a proper game window is found
void findGame(offset &addrs)
{
	addrs.game = NULL;
	while (!addrs.game)//Loop while addrs.game is NULL
	{
		if (FindWindow(NULL, "Sven Co-op"))
		{
			addrs.gameName = new char[strlen("Sven Co-op") + 1];
			strcpy(addrs.gameName, "Sven Co-op");
			addrs.game = FindWindow(NULL, addrs.gameName);
			addrs.SPECTATING = 0x2F2B08;
			addrs.ON_GROUND = 0x2EEB04;
			addrs.OVERLAY = 0x14374C;
			addrs.WATER = 0x5FAE6C;
		}
		else if (FindWindow(NULL, "Sven Co-op listen server - Sven Co-op"))
		{
			addrs.gameName = new char[strlen("Sven Co-op") + 1];
			strcpy(addrs.gameName, "Sven Co-op");
			addrs.game = FindWindow(NULL, "Sven Co-op listen server - Sven Co-op");
			addrs.SPECTATING = 0x2F2B08;
			addrs.ON_GROUND = 0x2EEB04;
			addrs.OVERLAY = 0x14374C;
			addrs.WATER = 0x5FAE6C;
		}
	}
	std::cout << "Found game: " << addrs.gameName << std::endl;
}

//Function to send a single space key press to the window
void sendSpace(const offset &addrs)
{
	SendMessage(addrs.game, WM_KEYUP, VK_SPACE, 0x390000);
	Sleep(10);
	SendMessage(addrs.game, WM_KEYDOWN, VK_SPACE, 0x390000);
}

//Function that loops and checks if the user is holding space
void bhopLoop(const offset &addrs)
{
	std::cout << "The tool is now active, \nIMPORTANT: Leave this window open while the tool is in use." << std::endl;
	int bhop_enable = 1;//bhop enable defaults to perfect
	int space = 0;
	DWORD pStatus;
	GetExitCodeProcess(worker.hProcess, &pStatus);
	srand(time(NULL));//Seed the random number generator

	while (pStatus == 259)
	{
		GetExitCodeProcess(worker.hProcess, &pStatus);
		Sleep(20);
		if (GetAsyncKeyState(VK_SPACE) && bhop_enable && onGround(addrs) && !overlayOpen(addrs) && !isSpectating(addrs))
		{
			space = 1;
			sendSpace(addrs);
			Sleep(20);
		}
		//reset space var to 0
		else if (!GetAsyncKeyState(VK_SPACE) && space){
			SendMessage(addrs.game, WM_KEYUP, VK_SPACE, 0x390000);
			space = 0;
		}
		//User is in water and not touching the ground, so simulate +jump until they are out of the water, touch ground, or let go of space
		else if (isInWater(addrs)&& GetAsyncKeyState(VK_SPACE) && !onGround(addrs))
		{
			SendMessage(addrs.game, WM_KEYDOWN, VK_SPACE, 0x390000);
			while (isInWater(addrs) && GetAsyncKeyState(VK_SPACE) && !onGround(addrs)){
				space = 1;
				Sleep(10);
			}
			SendMessage(addrs.game, WM_KEYUP, VK_SPACE, 0x390000);
			space = 0;
			
		}
	}
	system("cls");
}