#pragma once
#include	<cglm/call.h>

typedef struct	Screen_t	Screen;


Screen	*Screen_Create(GraphicsDevice *pGD, StuffKeeper *pSK,
						CBKeeper *pCBK, GrogFont *pGF,
						int targWidth, int targHeight,
						int screenWidth, int screenHeight);
void	Screen_Destroy(Screen **ppScr);

void	Screen_SetContents(Screen *pScr, GraphicsDevice *pGD,
							CBKeeper *pCBK, const uint8_t *pContents);

void	Screen_Draw(Screen *pScr, GraphicsDevice *pGD);