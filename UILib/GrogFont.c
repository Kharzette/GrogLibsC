#include	"d3d11.h"
#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<cglm/call.h>
#include	<assert.h>
#include	"utstring.h"


typedef struct	GrogFont_t
{
	int		mWidth, mHeight;
	int		mCellWidth, mCellHeight;
	uint8_t	mStartChar;

	ID3D11ShaderResourceView	*mpSRV;

	uint8_t	*mpWidths;
}	GrogFont;


GrogFont	*GFont_Create(const UT_string *pPath)
{
	if(pPath == NULL)
	{
		return	NULL;
	}

	FILE	*f	=fopen(utstring_body(pPath), "rb");
	if(f == NULL)
	{
		return	NULL;
	}

	GrogFont	*pRet	=malloc(sizeof(GrogFont));

	fread(&pRet->mWidth, sizeof(int), 1, f);
	fread(&pRet->mHeight, sizeof(int), 1, f);
	fread(&pRet->mCellWidth, sizeof(int), 1, f);
	fread(&pRet->mCellHeight, sizeof(int), 1, f);
	fread(&pRet->mStartChar, sizeof(uint8_t), 1, f);

	pRet->mpWidths	=malloc(255);

	fread(pRet->mpWidths, 1, 255, f);

	fclose(f);

	pRet->mpSRV	=NULL;

	return	pRet;
}

void	GFont_Destroy(GrogFont **ppFont)
{
	GrogFont	*pFont	=*ppFont;

	pFont->mpSRV->lpVtbl->Release(pFont->mpSRV);

	free(pFont->mpWidths);

	free(pFont);

	*ppFont	=NULL;
}


void	GFont_SetSRV(GrogFont *pFont, ID3D11ShaderResourceView *pSRV)
{
	if(pFont == NULL)
	{
		return;
	}

	//add a ref
	pSRV->lpVtbl->AddRef(pSRV);

	pFont->mpSRV	=pSRV;
}

ID3D11ShaderResourceView	*GFont_GetSRV(const GrogFont *pFont)
{
	if(pFont == NULL)
	{
		return	NULL;
	}

	return	pFont->mpSRV;
}

int	GFont_GetCharacterWidth(const GrogFont *pFont, char c)
{
	return	pFont->mpWidths[(int)c];
}

int	GFont_GetCharacterHeight(const GrogFont *pFont)
{
	return	pFont->mCellHeight;
}

void	GFont_GetUV(const GrogFont *pFont, char letter,
					int triIndex, vec2 uv)
{
	int	posOffset	=letter - pFont->mStartChar;
	if(posOffset < 0)
	{
		glm_vec2_copy(GLM_VEC2_ZERO, uv);
		return;
	}

	int	numColumns	=pFont->mWidth / pFont->mCellWidth;
	int	yOffset		=0;
	while(posOffset >= numColumns)
	{
		yOffset++;
		posOffset	-=numColumns;
	}

	uv[0]	=posOffset * pFont->mCellHeight;
	uv[1]	=yOffset * pFont->mCellHeight;

	int	charWidth	=GFont_GetCharacterWidth(pFont, letter);

	switch(triIndex)
	{
		case	0:
			break;
		case	1:
			uv[0]	+=charWidth;
			break;
		case	2:
			uv[0]	+=charWidth;
			uv[1]	+=pFont->mCellHeight;
			break;
		case	3:
			break;
		case	4:
			uv[0]	+=charWidth;
			uv[1]	+=pFont->mCellHeight;
			break;
		case	5:
			uv[1]	+=pFont->mCellHeight;
			break;
		default:
			assert(false);
			break;
	}

	uv[0]	/=pFont->mWidth;
	uv[1]	/=pFont->mHeight;
}

//Returns the longest line width in the string
float	GFont_MeasureTextClay(const GrogFont *pFont, const char *pText, int len)
{
	if(len <= 0)
	{
		return	0.0f;
	}

	float	maxWidth	=0.0f;
	float	lineWidth	=0.0f;

	for(int i=0;i < len;i++)
	{
		char	c	=pText[i];

		if(c == '\n')
		{
			maxWidth	=fmax(maxWidth, lineWidth);
			lineWidth	=0;
			continue;
		}
		lineWidth	+=GFont_GetCharacterWidth(pFont, c);
	}

	return	fmax(maxWidth, lineWidth);
}

//Returns the longest line width in the string
float	GFont_MeasureText(const GrogFont *pFont, const char *pText)
{
	int	len	=strlen(pText);

	return	GFont_MeasureTextClay(pFont, pText, len);
}