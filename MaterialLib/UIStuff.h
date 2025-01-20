#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	UIStuff_t			UIStuff;
typedef struct	GrogFont_t			GrogFont;

typedef struct	UIRect_t
{
	float	x, y;	//topleft position
	float	width, height;
}	UIRect;

UIStuff	*UI_Create(GraphicsDevice *pGD, StuffKeeper *pSK, int maxVerts);
void	UI_FreeAll(UIStuff *pUI);

void	UI_BeginDraw(UIStuff *pUI);
void	UI_EndDraw(UIStuff *pUI);

void	UI_DrawImage(UIStuff *pUI, const char *szTex, const vec2 pos,
					 float rotation, float scale, const vec4 color);
void	UI_DrawString(UIStuff *pUI, const char *pText, int len,
						GrogFont *pFont, const vec2 pos, const vec4 colour);
void	UI_DrawRect(UIStuff *pUI, const UIRect r, const vec4 color);
void	UI_DrawRectRounded(UIStuff *pUI, const UIRect r, float roundNess, int segments, const vec4 color);