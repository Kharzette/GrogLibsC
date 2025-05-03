#pragma once

typedef struct	EasyMenu_t	EasyMenu;

//callbacks
typedef void	(*EasyMenu_ItemChosenCB)(int idx, void *pContext);

EasyMenu	*EasyMenu_Create(int fontSize, int fontID, int numEntries);
void	EasyMenu_Destroy(EasyMenu **ppEM);

void	EasyMenu_AddEntry(EasyMenu *pEM, int idx, const char *pText,
							void *pContext, EasyMenu_ItemChosenCB cb);

void	EasyMenu_Draw(const EasyMenu *pEM);