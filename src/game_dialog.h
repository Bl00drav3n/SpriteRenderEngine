#ifndef __GAME_DIALOG_H__
#define __GAME_DIALOG_H__

#define MAX_CHARACTERS_PER_BOX 1024
#define MAX_LENGTH_NAME 35
#define MAX_TEXT_PER_DIALOG 20

struct dialog_text
{
	char* String[MAX_CHARACTERS_PER_BOX];
	char* Speaker[MAX_LENGTH_NAME];
	dialog_text* Next;
	dialog_text* Prev;
};

struct dialog_set
{
	dialog_text Content[MAX_TEXT_PER_DIALOG];
	dialog_set* Opt1;
	dialog_set* Opt2;
	//Add more options if neccessary...
};
#endif