#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	ScreenText_t		ScreenText;

ScreenText	*ST_Create(GraphicsDevice *pGD, const StuffKeeper *pSK, int maxChars,
						const char *pFontName, const char *pFontTexName);
void	ST_AddString(ScreenText *pST, const char *pSZ, const int id,
					const vec4 color, const vec2 pos, const vec2 scale);
void	ST_Update(ScreenText *pST, GraphicsDevice *pGD);
void	ST_Draw(ScreenText *pST, GraphicsDevice *pGD, CBKeeper *pCBK);

void	ST_FreeAll(ScreenText *pST);
void	ST_DeleteString(ScreenText *pST, int id);

void	ST_MeasureString(ScreenText *pST, const char *pToMeasure, vec2 size);
void	ST_ModifyStringScale(ScreenText *pST, int id, const vec2 scale);
void	ST_ModifyStringPosition(ScreenText *pST, int id, const vec2 pos);
void	ST_ModifyStringText(ScreenText *pST, int id, const char *pText);
void	ST_ModifyStringColor(ScreenText *pST, int id, const vec4 color);
