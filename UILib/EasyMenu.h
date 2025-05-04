#pragma once
#include 	"../clay/clay.h"

typedef struct	EasyMenu_t	EasyMenu;

//callbacks
typedef void	(*EasyMenu_ItemChosenCB)(int idx, void *pContext);

EasyMenu	*EasyMenu_Create(int fontSize, int fontID, int numEntries,
	Clay_Color textCol, Clay_Color bgCol,
	Clay_Color selTextCol, Clay_Color selBGCol);

void	EasyMenu_Destroy(EasyMenu **ppEM);

void	EasyMenu_AddEntry(EasyMenu *pEM, int idx, const char *pText,
							void *pContext, EasyMenu_ItemChosenCB cb);

void	EasyMenu_SelectUp(EasyMenu *pEM);
void	EasyMenu_SelectDown(EasyMenu *pEM);
int		EasyMenu_SelectChoice(EasyMenu *pEM);

void	EasyMenu_Draw(const EasyMenu *pEM);