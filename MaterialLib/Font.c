#include	<stdint.h>
#include	<stdio.h>
#include	<ctype.h>
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