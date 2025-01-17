#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	UIStuff_t			UIStuff;
typedef struct	GrogFont_t			GrogFont;

UIStuff	*UI_Create(GraphicsDevice *pGD, const StuffKeeper *pSK,
					GrogFont *pFont, int maxVerts);
void	UI_FreeAll(UIStuff *pUI);

void	UI_BeginDraw(UIStuff *pUI);
void	UI_EndDraw(UIStuff *pUI);

void	UI_DrawString(UIStuff *pUI, const char *pText, int len,
						GrogFont *pFont, const vec2 pos, const vec4 colour);
void	UI_DrawRect(UIStuff *pUI, float x, float y, float width,
					float height, const vec4 color);