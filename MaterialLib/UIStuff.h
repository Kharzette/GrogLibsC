#pragma once
#include	<stdint.h>
#include	<cglm/call.h>

typedef struct	GraphicsDevice_t	GraphicsDevice;
typedef struct	StuffKeeper_t		StuffKeeper;
typedef struct	CBKeeper_t			CBKeeper;
typedef struct	GrogText_t			GrogText;
typedef struct	GrogFont_t			GrogFont;

GrogText	*GText_Create(GraphicsDevice *pGD, const StuffKeeper *pSK, int maxChars,
						const char *pFontName, const char *pFontTexName);

void	GText_Draw(GrogText *pGT, GraphicsDevice *pGD, CBKeeper *pCBK,
					const char *pText, const GrogFont *pFont,
					vec2 pos, float size, vec4 colour);

void	GText_FreeAll(GrogText *pST);

//Returns the longest line width in the string
float	GText_MeasureText(const GrogText *pGT, const GrogFont *pFont, const char *pText);