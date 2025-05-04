//be sure to define CLAY_IMPLEMENTATION in a single
//spot before including this, in whatever exe is being made
#pragma once
#include	<stdint.h>
#include	<cglm/call.h>
#include 	"../clay/clay.h"

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
					 const vec2 size, float rotation, const vec4 color);
void	UI_DrawString(UIStuff *pUI, const char *pText, int len,
						GrogFont *pFont, const vec2 pos, const vec4 colour);
void	UI_DrawRect(UIStuff *pUI, const UIRect r, const vec4 color);
void	UI_DrawRectHollow(UIStuff *pUI, const UIRect r, int hullSize, const vec4 color);
void	UI_DrawRectRounded(UIStuff *pUI, const UIRect r, float roundNess,
						   int segments, const vec4 color);
void	UI_DrawRRHollow(UIStuff *pUI, const UIRect r, float hullSize,
						float roundNess, int segments, const vec4 color);

//font helper stuff
void		UI_AddAllFonts(UIStuff *pUI);
uint16_t	UI_GetNearestFontSize(const UIStuff *pUI, int size);
int			UI_GetFontSize(const UIStuff *pUI, uint16_t id);

//clay specific stuff
void	UI_AddFont(UIStuff *pUI, const char *szFontName, uint16_t id);
void	UI_ClayRender(UIStuff *pUI, Clay_RenderCommandArray renderCommands);
void	UI_ClayColorToVec4(Clay_Color in, vec4 out);

Clay_Dimensions	UI_MeasureText(Clay_StringSlice pText,
	Clay_TextElementConfig *pConfig, void *pUserData);
