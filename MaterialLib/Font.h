#pragma once
#include	"utstring.h"
#include	<cglm/call.h>

typedef struct	Font_t	Font;

Font	*Font_Create(UT_string *pPath);

int		Font_GetCharacterWidth(const Font *pFont, char c);
int		Font_GetCharacterHeight(const Font *pFont);
void	Font_GetUV(const Font *pFont, char letter, int triIndex, vec2 uv);