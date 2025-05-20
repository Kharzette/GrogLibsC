#include	<stdint.h>
#include	"Layouts.h"
#include	"utstring.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"


typedef struct	ShaderBytes_t
{
	uint8_t	*mpBytes;
	int		mLen;
}	ShaderBytes;

typedef struct	Match_t
{
	//search criteria
	const int	*mpElements;
	int			mElementCount;

	const DictSZ	*mpLayCounts;

	//the match
	const UT_string	*mpKey;
}	Match;


static void	sAddElCopy(DictSZ **ppElems, DictSZ **ppLaySizes,
	const char *pName, int elems[], int elCount)
{
	//make a mallocd copy of the elems
	int	*pElCopy	=malloc(sizeof(int) * elCount);
	memcpy(pElCopy, elems, sizeof(int) * elCount);

	//storing an int in a void * spot
	int64_t	bigElCount	=elCount;

	DictSZ_Addccp(ppElems, pName, pElCopy);
	DictSZ_Addccp(ppLaySizes, pName, (void *)bigElCount);
}


void	Layouts_MakeElems(DictSZ **ppElems, DictSZ **ppLaySizes)
{
	//UIVert
	sAddElCopy(ppElems, ppLaySizes, "UIVert", (int []){EL_POSITION2, EL_BINDICES}, 2);

	//VPos2Tex0
	sAddElCopy(ppElems, ppLaySizes, "VPos2Tex0", (int []){EL_POSITION2, EL_TEXCOORD}, 2);

	//VPos2Tex04
	sAddElCopy(ppElems, ppLaySizes, "VPos2Tex04", (int []){EL_POSITION2, EL_TEXCOORD4}, 2);

	//VPos2Col0
	sAddElCopy(ppElems, ppLaySizes, "VPos2Col0", (int []){EL_POSITION2, EL_COLOR}, 2);

	//VPos2Col0Tex04
	sAddElCopy(ppElems, ppLaySizes, "VPos2Col0Tex04",
		(int []){EL_POSITION2, EL_COLOR, EL_TEXCOORD4}, 3);
}

void	Layouts_MakeLayouts(GraphicsDevice *pGD, const DictSZ *pVSCode,
							DictSZ **ppLayouts)
{
	//VPos2Tex0
	D3D11_INPUT_ELEMENT_DESC	iedVPos2Tex0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,	0,	8,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos2Tex04
	D3D11_INPUT_ELEMENT_DESC	iedVPos2Tex04[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	8,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos2Col0
	D3D11_INPUT_ELEMENT_DESC	iedVPos2Col0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	8,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos2Col0Tex04
	D3D11_INPUT_ELEMENT_DESC	iedVPos2Col0Tex04[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	8,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	16,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos2Tex0
	ShaderBytes			*pCode	=DictSZ_GetValueccp(pVSCode, "TextVS");
	ID3D11InputLayout	*pLO	=GD_CreateInputLayout(pGD, iedVPos2Tex0, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Tex0", pLO);

	//VPos2Tex04
	pCode	=DictSZ_GetValueccp(pVSCode, "KeyedGumpVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos2Tex04, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Tex04", pLO);

	//VPos2Col0
	pCode	=DictSZ_GetValueccp(pVSCode, "ShapeVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos2Col0, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Col0", pLO);

	//VPos2Col0Tex04
	pCode	=DictSZ_GetValueccp(pVSCode, "UIStuffVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos2Col0Tex04, 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Col0Tex04", pLO);
}


static void	sForEachEDesc(const UT_string *pKey, const void *pValue, void *pContext)
{
	int	*pElems	=(int *)pValue;
	if(pElems == NULL)
	{
		return;
	}

	Match	*pM	=(Match *)pContext;
	if(pM == NULL)
	{
		return;
	}

	int64_t	elCount	=(int64_t)DictSZ_GetValue(pM->mpLayCounts, pKey);

	if(elCount != pM->mElementCount)
	{
		return;
	}

	for(int i=0;i < pM->mElementCount;i++)
	{
		if(pElems[i] != pM->mpElements[i])
		{
			return;	//no match
		}
	}
	
	//ensure no earlier match
	assert(pM->mpKey == NULL);

	//match!
	pM->mpKey	=pKey;
}


const UT_string	*Layouts_FindMatch(
	const DictSZ *pLayCounts, const DictSZ *pElems,
	const int elems[], int elCount)
{
	Match	m	={	elems, elCount, pLayCounts, NULL	};

	DictSZ_ForEach(pElems, sForEachEDesc, &m);

	return	m.mpKey;
}

void	Layouts_GetGrogSizes(const int elems[], int sizes[], int elCount)
{
	for(int i=0;i < elCount;i++)
	{
		switch(elems[i])
		{
			case	EL_TEXCOORD:
			case	EL_BINDICES:
				sizes[i]	=4;
				break;
			case	EL_BWEIGHTS:
			case	EL_COLOR:
			case	EL_NORMAL:
			case	EL_TANGENT:
			case	EL_TEXCOORD4:
			case	EL_POSITION2:
				sizes[i]	=8;
				break;
			case	EL_POSITION4:
				sizes[i]	=16;
				break;
			case	EL_POSITION:
				sizes[i]	=12;
				break;
		}
	}
}