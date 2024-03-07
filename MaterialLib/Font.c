#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<cglm/call.h>
#include	<assert.h>
#include	"utstring.h"


typedef struct	Font_t
{
	int		mWidth, mHeight;
	int		mCellWidth, mCellHeight;
	uint8_t	mStartChar;

	uint8_t	*mpWidths;
}	Font;


Font	*Font_Create(UT_string *pPath)
{
	FILE	*f	=fopen(utstring_body(pPath), "rb");
	if(f == NULL)
	{
		return	NULL;
	}

	Font	*pRet	=malloc(sizeof(Font));

	fread(&pRet->mWidth, sizeof(int), 1, f);
	fread(&pRet->mHeight, sizeof(int), 1, f);
	fread(&pRet->mCellWidth, sizeof(int), 1, f);
	fread(&pRet->mCellHeight, sizeof(int), 1, f);
	fread(&pRet->mStartChar, sizeof(uint8_t), 1, f);

	pRet->mpWidths	=malloc(255);

	fread(pRet->mpWidths, 1, 255, f);

	fclose(f);

	return	pRet;
}

int	Font_GetCharacterWidth(const Font *pFont, char c)
{
	return	pFont->mpWidths[(int)c];
}

int	Font_GetCharacterHeight(const Font *pFont)
{
	return	pFont->mCellHeight;
}

void	Font_GetUV(const Font *pFont, char letter, int triIndex, vec2 uv)
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

	int	charWidth	=Font_GetCharacterWidth(pFont, letter);

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