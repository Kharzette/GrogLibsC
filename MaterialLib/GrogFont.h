#pragma once
#include	"utstring.h"
#include	<cglm/call.h>

typedef struct	GrogFont_t	GrogFont;
typedef interface ID3D11ShaderResourceView	ID3D11ShaderResourceView;

GrogFont	*Font_Create(const UT_string *pPath);

void	Font_SetSRV(GrogFont *pFont, ID3D11ShaderResourceView *pSRV);

int		Font_GetCharacterWidth(const GrogFont *pFont, char c);
int		Font_GetCharacterHeight(const GrogFont *pFont);
void	Font_GetUV(const GrogFont *pFont, char letter, int triIndex, vec2 uv);

ID3D11ShaderResourceView *Font_GetSRV(const GrogFont *pFont);