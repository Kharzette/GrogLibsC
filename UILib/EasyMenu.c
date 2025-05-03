#include	<string.h>
#include	"../UILib/UIStuff.h"
#include	"EasyMenu.h"


typedef struct	EasyMenu_t
{
	int	mNumEntries;
	int	mFontSize;
	int	mFontID;

	char	**mppMenuText;
	void	**mppContexts;
	int		*mpLens;

	EasyMenu_ItemChosenCB	*mpCBs;

}	EasyMenu;

//static forward decs
static void	sDrawMenuItem(const EasyMenu *pEM, int index);


EasyMenu	*EasyMenu_Create(int fontSize, int fontID, int numEntries)
{
	EasyMenu	*pRet	=malloc(sizeof(EasyMenu));

	memset(pRet, 0, sizeof(EasyMenu));

	pRet->mFontSize		=fontSize;
	pRet->mFontID		=fontID;
	pRet->mNumEntries	=numEntries;

	pRet->mppMenuText	=malloc(sizeof(char *) * numEntries);
	pRet->mppContexts	=malloc(sizeof(void *) * numEntries);
	pRet->mpLens		=malloc(sizeof(int) * numEntries);
	pRet->mpCBs			=malloc(sizeof(EasyMenu_ItemChosenCB *) * numEntries);

	//zero out arrays
	memset(pRet->mppMenuText, 0, sizeof(char *) * numEntries);
	memset(pRet->mppContexts, 0, sizeof(void *) * numEntries);
	memset(pRet->mpLens, 0, sizeof(int) * numEntries);
	memset(pRet->mpCBs, 0, sizeof(EasyMenu_ItemChosenCB *) * numEntries);

	return	pRet;
}

void	EasyMenu_Destroy(EasyMenu **ppEM)
{
	EasyMenu	*pEM	=*ppEM;

	for(int i=0;i < pEM->mNumEntries;i++)
	{
		free(pEM->mppMenuText[i]);
	}

	free(pEM->mpCBs);
	free(pEM->mppContexts);
	free(pEM->mppMenuText);
	free(pEM->mpLens);

	*ppEM	=NULL;
}


void	EasyMenu_AddEntry(EasyMenu *pEM, int idx, const char *pText,
						void *pContext, EasyMenu_ItemChosenCB cb)
{
	assert(pEM);
	assert(idx >= 0 && idx < pEM->mNumEntries);
	assert(pText);
	assert(cb);

	if(pEM->mpCBs[idx] != NULL
		|| pEM->mppContexts[idx] != NULL
		|| pEM->mppMenuText[idx] != NULL)
	{
		printf("EasyMenu: Attempt to add an already used index!");
		return;
	}

	int	len	=strlen(pText);

	assert(len > 0);

	pEM->mpLens[idx]	=len;

	pEM->mppMenuText[idx]	=malloc(len + 1);

	strncpy(pEM->mppMenuText[idx], pText, len);

	//terminate
	pEM->mppMenuText[idx][len]	=0;

	pEM->mpCBs[idx]			=cb;
	pEM->mppContexts[idx]	=pContext;
}

void	EasyMenu_Draw(const EasyMenu *pEM)
{
	assert(pEM);

	for(int i=0;i < pEM->mNumEntries;i++)
	{
		sDrawMenuItem(pEM, i);
	}
}


static void	sDrawMenuItem(const EasyMenu *pEM, int index)
{
	Clay_String	mmText;

	mmText.chars	=pEM->mppMenuText[index];
	mmText.length	=pEM->mpLens[index];

	mmText.isStaticallyAllocated	=false;

	CLAY({
		.layout =
		{
			.childAlignment	=
			{
				.x	= CLAY_ALIGN_X_CENTER, .y	= CLAY_ALIGN_Y_CENTER
			},
			.layoutDirection	=CLAY_TOP_TO_BOTTOM,
			.sizing	=
			{
				.width = CLAY_SIZING_FIT(0),
				.height = CLAY_SIZING_FIT(0)
			},
			.padding = {16,16,16,16},
			.childGap = 16
		},
		.border =
		{
			.color = {80, 80, 80, 255},
			.width	=CLAY_BORDER_ALL(2)
		},
		.backgroundColor	={150, 150, 155, 55	}
	})
	{
		CLAY_TEXT(mmText, CLAY_TEXT_CONFIG(
		{
			.fontSize	=pEM->mFontSize,
			.fontId		=pEM->mFontID,
			.textColor	={0, 70, 70, 155}
		}));
	}	
}