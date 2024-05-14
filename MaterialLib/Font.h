#pragma once
#include	"utstring.h"
#include	<cglm/call.h>

typedef struct	GrogFont_t	GrogFont;

GrogFont	*Font_Create(UT_string *pPath);

int		Font_GetCharacterWidth(const GrogFont *pFont, char c);
int		Font_GetCharacterHeight(const GrogFont *pFont);
void	Font_GetUV(const GrogFont *pFont, char letter, int triIndex, vec2 uv);