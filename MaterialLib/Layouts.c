#include	<stdint.h>
#include	"d3d11.h"
#include	"utstring.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"


typedef struct	ShaderBytes_t
{
	uint8_t	*mpBytes;
	int		mLen;
}	ShaderBytes;


void	MakeLayouts(GraphicsDevice *pGD, DictSZ **ppLayouts, DictSZ *pVSCode)
{
	//vpos
	D3D11_INPUT_ELEMENT_DESC	iedVPos[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPosTex0
	D3D11_INPUT_ELEMENT_DESC	iedVPosTex0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,		0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	//VPos2Tex02
	D3D11_INPUT_ELEMENT_DESC	iedVPos2Tex02[]	=
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

	//VPosNormTex0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16_FLOAT,		0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
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

	//VPosNormTex04Tex14Tex24Col04
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex04Tex14Tex24Col04[]	=
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

	//VPosNormTex04Col0
	D3D11_INPUT_ELEMENT_DESC	iedVPosNormTex04Col0[]	=
	{
		{	"POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"NORMAL",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"TEXCOORD",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	20,	D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{	"COLOR",	0,	DXGI_FORMAT_R16G16B16A16_FLOAT,	0,	28,	D3D11_INPUT_PER_VERTEX_DATA, 0	}
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

	//VPosTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "SkyVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosTex0, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosTex0", pLO);

	//VPos2Tex02
	pCode	=DictSZ_GetValueccp(pVSCode, "TextVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos2Tex02, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Tex02", pLO);

	//VPos2Tex04
	pCode	=DictSZ_GetValueccp(pVSCode, "KeyedGumpVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPos2Tex04, 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Tex04", pLO);

	//VPosNormTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "TexTriVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex0, 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex0", pLO);

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

	//VPosNormTex04Tex14Tex24Col04
	pCode	=DictSZ_GetValueccp(pVSCode, "LightMapAnimVS");
	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex04Tex14Tex24Col04, 6, pCode->mpBytes, pCode->mLen);
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

	//VPosNormTex04Col0, do when terrain gets added
//	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosTexFactVS");
//	pLO		=GD_CreateInputLayout(pGD, iedVPosNormTex04Col0, 4, pCode->mpBytes, pCode->mLen);
//	if(pLO == NULL)
//	{
//		printf("Error creating layout.\n");
//		return;
//	}
//	DictSZ_Addccp(ppLayouts, "VPosNormTex04Col0", pLO);
}