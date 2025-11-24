#pragma once
#include	"utstring.h"
#include	<cglm/call.h>

typedef struct	GrogFont_t	GrogFont;
typedef interface ID3D11ShaderResourceView	ID3D11ShaderResourceView;

GrogFont	*GFont_Create(const UT_string *pPath);
void		GFont_Destroy(GrogFont **ppFont);

void	GFont_SetSRV(GrogFont *pFont, ID3D11ShaderResourceView *pSRV);

int		GFont_GetCharacterWidth(const GrogFont *pFont, char c);
int		GFont_GetCharacterHeight(const GrogFont *pFont);
void	GFont_GetUV(const GrogFont *pFont, char letter, int triIndex, vec2 uv);

//Returns the longest line width in the string
float	GFont_MeasureText(const GrogFont *pFont, const char *pText);
float	GFont_MeasureTextClay(const GrogFont *pFont, const char *pText, int len);

ID3D11ShaderResourceView *GFont_GetSRV(const GrogFont *pFont);