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
	//vpos
	sAddElCopy(ppElems, ppLaySizes, "VPos", (int []){EL_POSITION}, 1);
	
	//VPosNorm
	sAddElCopy(ppElems, ppLaySizes, "VPosNorm", (int []){EL_POSITION, EL_NORMAL}, 2);

	//VPosTex0
	sAddElCopy(ppElems, ppLaySizes, "VPosTex0", (int []){EL_POSITION, EL_TEXCOORD}, 2);

	//VPos2Tex0
	sAddElCopy(ppElems, ppLaySizes, "VPos2Tex0", (int []){EL_POSITION2, EL_TEXCOORD}, 2);

	//VPos2Tex04
	sAddElCopy(ppElems, ppLaySizes, "VPos2Tex04", (int []){EL_POSITION2, EL_TEXCOORD4}, 2);

	//VPos2Col0
	sAddElCopy(ppElems, ppLaySizes, "VPos2Col0", (int []){EL_POSITION2, EL_COLOR}, 2);

	//VPos2Col0Tex04
	sAddElCopy(ppElems, ppLaySizes, "VPos2Col0Tex04",
		(int []){EL_POSITION2, EL_COLOR, EL_TEXCOORD4}, 3);

	//VPosNormTex0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTex0",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD}, 3);

	//VPosNormCol0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormCol0",
		(int []){EL_POSITION, EL_NORMAL, EL_COLOR}, 3);

	//VPosNormTex04
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTex04",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD4}, 3);

	//VPosNormTex0Col0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTex0Col0",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD, EL_COLOR}, 4);

	//VPos4Tex04Tex14
	sAddElCopy(ppElems, ppLaySizes, "VPos4Tex04Tex14",
		(int []){EL_POSITION4, EL_TEXCOORD4, EL_TEXCOORD4}, 4);

	//VPosNormTex04Tex14
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTex04Tex14",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD4, EL_TEXCOORD4}, 4);

	//VPosNormTex04Tex14Tex24Col0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTex04Tex14Tex24Col0",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD4,
			EL_TEXCOORD4, EL_TEXCOORD4, EL_COLOR}, 6);

	//VPosNormBone
	sAddElCopy(ppElems, ppLaySizes, "VPosNormBone",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS}, 4);

	//VPosNormBoneCol0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormBoneCol0",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS, EL_COLOR}, 5);

	//VPosNormBoneTex0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormBoneTex0",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS, EL_TEXCOORD}, 5);

	//VPosNormBoneTex0Tex1
	sAddElCopy(ppElems, ppLaySizes, "VPosNormBoneTex0Tex1",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS,
			EL_TEXCOORD4, EL_TEXCOORD4}, 6);

	//VPosNormTanTex0
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTanTex0",
		(int []){EL_POSITION, EL_NORMAL, EL_TANGENT, EL_TEXCOORD}, 4);

	//VPosNormTex0Bone	newer wacky order
	sAddElCopy(ppElems, ppLaySizes, "VPosNormTex0Bone",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD, EL_BINDICES, EL_BWEIGHTS}, 5);
}

void	Layouts_MakeLayouts(GraphicsDevice *pGD, const DictSZ *pVSCode,
							DictSZ **ppLayouts)
{
	//vpos
	D3D11_INPUT_ELEMENT_DESC	iedVPos[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNorm
	D3D11_INPUT_ELEMENT_DESC	iedVPosNorm[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosTex0
	D3D11_INPUT_ELEMENT_DESC	iedVPosTex0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,		0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

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

	//VPosNormTex0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormCol0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormCol0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormTex04
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex04[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormTex0Col0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex0Col0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos4Tex04Tex14
	D3D11_INPUT_ELEMENT_DESC	iedVPos4Tex04Tex14[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	16,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	1,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormTex04Tex14
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex04Tex14[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	1,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	28,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormTex04Tex14Tex24Col0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex04Tex14Tex24Col0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	1,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	28,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	2,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	36,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	44,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormBone - Note, testing byte4 indexes
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormBone[]	=
	{
		{	"POSITION",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDINDICES",	0,	DXGI_FORMAT_R8G8B8A8_UINT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDWEIGHTS",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormBoneCol0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormBoneCol0[]	=
	{
		{	"POSITION",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDINDICES",	0,	DXGI_FORMAT_R8G8B8A8_UINT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDWEIGHTS",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	32,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormBoneTex0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormBoneTex0[]	=
	{
		{	"POSITION",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDINDICES",	0,	DXGI_FORMAT_R8G8B8A8_UINT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDWEIGHTS",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",		0,	DXGI_FORMAT_R16G16_FLOAT,		0,	32,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormBoneTex0Tex1
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormBoneTex0Tex1[]	=
	{
		{	"POSITION",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDINDICES",	0,	DXGI_FORMAT_R8G8B8A8_UINT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDWEIGHTS",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	32,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",		1,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	40,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormTanTex0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTanTex0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TANGENT",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,		0,	28,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosNormTex0Bone	newer wacky order
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex0Bone[]	=
	{
		{	"POSITION",		0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",		0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",		0,	DXGI_FORMAT_R16G16_FLOAT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDINDICES",	0,	DXGI_FORMAT_R8G8B8A8_UINT,		0,	24,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"BLENDWEIGHTS",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	28,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos
	ShaderBytes			*pCode	=DictSZ_GetValueccp(pVSCode, "WPosVS");
	ID3D11InputLayout	*pLO	=GD_CreateInputLayout(pGD, iedVPos, 1, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos", pLO);

	//VPosNorm
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNorm, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNorm", pLO);

	//VPosTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "SkyVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosTex0, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosTex0", pLO);

	//VPos2Tex0
	pCode	=DictSZ_GetValueccp(pVSCode, "TextVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos2Tex0, 2, pCode->mpBytes, pCode->mLen);
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

	//VPosNormTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "TexTriVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex0, 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex0", pLO);

	//VPosNormCol0
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosVColorVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormCol0, 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormCol0", pLO);

	//VPosNormTex04
	pCode	=DictSZ_GetValueccp(pVSCode, "LightMapVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex04, 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex04", pLO);

	//VPosNormTex0Col0
	pCode	=DictSZ_GetValueccp(pVSCode, "VertexLitVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex0Col0, 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex0Col0", pLO);

	//VPos4Tex04Tex14
	pCode	=DictSZ_GetValueccp(pVSCode, "ParticleVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos4Tex04Tex14, 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos4Tex04Tex14", pLO);

	//VPosNormTex04Tex14
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosTexFactVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex04Tex14, 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex04Tex14", pLO);

	//VPosNormTex04Tex14Tex24Col0
	pCode	=DictSZ_GetValueccp(pVSCode, "LightMapAnimVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex04Tex14Tex24Col0, 6, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex04Tex14Tex24Col04", pLO);

	//VPosNormBone
	pCode	=DictSZ_GetValueccp(pVSCode, "DMNVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormBone, 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBone", pLO);

	//VPosNormBoneCol0
	pCode	=DictSZ_GetValueccp(pVSCode, "DMNDanglyVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormBoneCol0, 5, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBoneCol0", pLO);

	//VPosNormBoneTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "SkinTexTriColVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormBoneTex0, 5, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBoneTex0", pLO);

	//VPosNormBoneTex0Tex1
	pCode	=DictSZ_GetValueccp(pVSCode, "SkinTex0Tex1TriColVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormBoneTex0Tex1, 6, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBoneTex0Tex1", pLO);
	
	//VPosNormTanTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWTanBTanWPosVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTanTex0, 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTanTex0", pLO);

	//VPosNormTex0Bone	new and wacky
	pCode	=DictSZ_GetValueccp(pVSCode, "SkinTexTriColVS2");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex0Bone, 5, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex0Bone", pLO);
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