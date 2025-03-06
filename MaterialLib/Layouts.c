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
	int	*mpElements;
	int	mElementCount;

	DictSZ	**mppLayCounts;

	//the match
	UT_string	*mpKey;
}	Match;


static void	sSetSemantic(D3D11_INPUT_ELEMENT_DESC *pIED, const char *pSem)
{
	int	len	=strlen(pSem);

	pIED->SemanticName	=malloc(len + 1);

	strncpy(pIED->SemanticName, pSem, len);
}

//I stole this from Chuck / DirectXMesh
static size_t sBytesPerElement(DXGI_FORMAT fmt)
{
    //This list only includes those formats that are valid for use by IB or VB

    switch(fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 16;

    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 12;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return 8;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return 4;

    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 2;

    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        return 1;

    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 2;

    default:
        // No BC, sRGB, XRBias, SharedExp, Typeless, Depth, or Video formats
        return 0;
    }
}

static int	sSetupIED(D3D11_INPUT_ELEMENT_DESC *pIED, int idx,
	int semIdx, const char *pSem, int ofs, DXGI_FORMAT fmt)
{
	sSetSemantic(&pIED[idx], pSem);

	pIED[idx].SemanticIndex			=semIdx;
	pIED[idx].Format				=fmt;
	pIED[idx].InputSlot				=0;
	pIED[idx].AlignedByteOffset		=ofs;
	pIED[idx].InputSlotClass		=D3D11_INPUT_PER_VERTEX_DATA;
	pIED[idx].InstanceDataStepRate	=0;

	return	sBytesPerElement(fmt);
}


static void	sAddADesc(DictSZ **ppDescs, DictSZ **ppElems,
	const char *vName, int elems[], int elCount)
{
	size_t	iedSize	=sizeof(D3D11_INPUT_ELEMENT_DESC);

	D3D11_INPUT_ELEMENT_DESC	*pIED	=malloc(iedSize * elCount);

	int	ofs	=0;
	for(int i=0;i < elCount;i++)
	{
		switch(elems[i])
		{
			case	EL_POSITION:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "POSITION", ofs,
					DXGI_FORMAT_R32G32B32_FLOAT);
			}
			break;
			case	EL_NORMAL:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "NORMAL", ofs,
					DXGI_FORMAT_R16G16B16A16_FLOAT);
			}
			break;
			case	EL_TEXCOORD:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "TEXCOORD", ofs,
					DXGI_FORMAT_R16G16_FLOAT);
			}
			break;
			case	EL_COLOR:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "COLOR", ofs,
					DXGI_FORMAT_R16G16B16A16_FLOAT);
			}
			break;
			case	EL_BINDICES:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "BLENDINDICES", ofs,
					DXGI_FORMAT_R8G8B8A8_UINT);
			}
			break;
			case	EL_BWEIGHTS:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "BLENDWEIGHTS", ofs,
					DXGI_FORMAT_R16G16B16A16_FLOAT);
			}
			break;
			case	EL_TANGENT:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "TANGENT", ofs,
					DXGI_FORMAT_R16G16B16A16_FLOAT);
			}
			break;
			case	EL_TEXCOORD4:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "TEXCOORD", ofs,
					DXGI_FORMAT_R16G16B16A16_FLOAT);
			}
			break;
			case	EL_POSITION2:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "POSITION", ofs,
					DXGI_FORMAT_R32G32_FLOAT);
			}
			break;
			case	EL_POSITION4:
			{
				ofs	+=sSetupIED(pIED, 0, 0, "POSITION", ofs,
					DXGI_FORMAT_R32G32B32A32_FLOAT);
			}
			break;
			default:
				assert(false);
		}
	}

	//make a mallocd copy of the elems
	int	*pElCopy	=malloc(sizeof(int) * elCount);
	memcpy(pElCopy, elems, sizeof(int) * elCount);

	DictSZ_Addccp(ppDescs, vName, pIED);
	DictSZ_Addccp(ppElems, vName, pElCopy);
}

void	MakeDescs(DictSZ **ppDescs, DictSZ **ppElems)
{
	//vpos
	sAddADesc(ppDescs, ppElems, "VPos", (int []){EL_POSITION}, 1);
	
	//VPosNorm
	sAddADesc(ppDescs, ppElems, "VPosNorm", (int []){EL_POSITION, EL_NORMAL}, 2);

	//VPosTex0
	sAddADesc(ppDescs, ppElems, "VPosTex0", (int []){EL_POSITION, EL_TEXCOORD}, 2);

	//VPos2Tex0
	sAddADesc(ppDescs, ppElems, "VPos2Tex0", (int []){EL_POSITION2, EL_TEXCOORD}, 2);

	//VPos2Tex04
	sAddADesc(ppDescs, ppElems, "VPos2Tex04", (int []){EL_POSITION2, EL_TEXCOORD4}, 2);

	//VPos2Col0
	sAddADesc(ppDescs, ppElems, "VPos2Col0", (int []){EL_POSITION2, EL_COLOR}, 2);

	//VPos2Col0Tex04
	sAddADesc(ppDescs, ppElems, "VPos2Col0Tex04",
		(int []){EL_POSITION2, EL_COLOR, EL_TEXCOORD4}, 3);

	//VPosNormTex0
	sAddADesc(ppDescs, ppElems, "VPosNormTex0",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD}, 3);

	//VPosNormCol0
	sAddADesc(ppDescs, ppElems, "VPosNormCol0",
		(int []){EL_POSITION, EL_NORMAL, EL_COLOR}, 3);

	//VPosNormTex04
	sAddADesc(ppDescs, ppElems, "VPosNormTex04",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD4}, 3);

	//VPosNormTex0Col0
	sAddADesc(ppDescs, ppElems, "VPosNormTex0Col0",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD, EL_COLOR}, 4);

	//VPos4Tex04Tex14
	sAddADesc(ppDescs, ppElems, "VPos4Tex04Tex14",
		(int []){EL_POSITION4, EL_TEXCOORD4, EL_TEXCOORD4}, 4);

	//VPosNormTex04Tex14
	sAddADesc(ppDescs, ppElems, "VPosNormTex04Tex14",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD4, EL_TEXCOORD4}, 4);

	//VPosNormTex04Tex14Tex24Col0
	sAddADesc(ppDescs, ppElems, "VPosNormTex04Tex14Tex24Col0",
		(int []){EL_POSITION, EL_NORMAL, EL_TEXCOORD4,
			EL_TEXCOORD4, EL_TEXCOORD4, EL_COLOR}, 6);

	//VPosNormBone
	sAddADesc(ppDescs, ppElems, "VPosNormBone",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS}, 4);

	//VPosNormBoneCol0
	sAddADesc(ppDescs, ppElems, "VPosNormBoneCol0",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS, EL_COLOR}, 5);

	//VPosNormBoneTex0
	sAddADesc(ppDescs, ppElems, "VPosNormBoneTex0",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS, EL_TEXCOORD}, 5);

	//VPosNormBoneTex0Tex1
	sAddADesc(ppDescs, ppElems, "VPosNormBoneTex0Tex1",
		(int []){EL_POSITION, EL_NORMAL, EL_BINDICES, EL_BWEIGHTS,
			EL_TEXCOORD4, EL_TEXCOORD4}, 6);

	//VPosNormTanTex0
	sAddADesc(ppDescs, ppElems, "VPosNormTanTex0",
		(int []){EL_POSITION, EL_NORMAL, EL_TANGENT, EL_TEXCOORD}, 4);
}


void	MakeLayouts(GraphicsDevice *pGD, const DictSZ *ppDescs,
	const DictSZ *pVSCode, DictSZ **ppLayouts, DictSZ **ppLaySizes)
{
	//VPos
	ShaderBytes			*pCode	=DictSZ_GetValueccp(pVSCode, "WPosVS");
	ID3D11InputLayout	*pLO	=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPos"), 1, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos", pLO);
	DictSZ_Addccp(ppLaySizes, "VPos", 1);

	//VPosNorm
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNorm"), 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNorm", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNorm", 2);

	//VPosTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "SkyVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosTex0"), 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosTex0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosTex0", 2);

	//VPos2Tex0
	pCode	=DictSZ_GetValueccp(pVSCode, "TextVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPos2Tex0"), 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Tex0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPos2Tex0", 2);

	//VPos2Tex04
	pCode	=DictSZ_GetValueccp(pVSCode, "KeyedGumpVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPos2Tex04"), 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Tex04", pLO);
	DictSZ_Addccp(ppLaySizes, "VPos2Tex04", 2);

	//VPos2Col0
	pCode	=DictSZ_GetValueccp(pVSCode, "ShapeVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPos2Col0"), 2, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Col0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPos2Col0", 2);

	//VPos2Col0Tex04
	pCode	=DictSZ_GetValueccp(pVSCode, "UIStuffVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPos2Col0Tex04"), 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos2Col0Tex04", pLO);
	DictSZ_Addccp(ppLaySizes, "VPos2Col0Tex04", 3);

	//VPosNormTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "TexTriVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormTex0"), 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormTex0", 3);

	//VPosNormCol0
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosVColorVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormCol0"), 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormCol0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormCol0", 3);

	//VPosNormTex04
	pCode	=DictSZ_GetValueccp(pVSCode, "LightMapVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormTex04"), 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex04", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormTex04", 3);

	//VPosNormTex0Col0
	pCode	=DictSZ_GetValueccp(pVSCode, "VertexLitVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormTex0Col0"), 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex0Col0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormTex0Col0", 4);

	//VPos4Tex04Tex14
	pCode	=DictSZ_GetValueccp(pVSCode, "ParticleVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPos4Tex04Tex14"), 3, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPos4Tex04Tex14", pLO);
	DictSZ_Addccp(ppLaySizes, "VPos4Tex04Tex14", 3);

	//VPosNormTex04Tex14
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWPosTexFactVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormTex04Tex14"), 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex04Tex14", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormTex04Tex14", 4);

	//VPosNormTex04Tex14Tex24Col0
	pCode	=DictSZ_GetValueccp(pVSCode, "LightMapAnimVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormTex04Tex14Tex24Col0"), 6, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTex04Tex14Tex24Col0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormTex04Tex14Tex24Col0", 6);

	//VPosNormBone
	pCode	=DictSZ_GetValueccp(pVSCode, "DMNVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormBone"), 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBone", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormBone", 4);

	//VPosNormBoneCol0
	pCode	=DictSZ_GetValueccp(pVSCode, "DMNDanglyVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormBoneCol0"), 5, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBoneCol0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormBoneCol0", 5);

	//VPosNormBoneTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "SkinTexTriColVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormBoneTex0"), 5, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBoneTex0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormBoneTex0", 5);

	//VPosNormBoneTex0Tex1
	pCode	=DictSZ_GetValueccp(pVSCode, "SkinTex0Tex1TriColVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormBoneTex0Tex1"), 6, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormBoneTex0Tex1", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormBoneTex0Tex1", 6);
	
	//VPosNormTanTex0
	pCode	=DictSZ_GetValueccp(pVSCode, "WNormWTanBTanWPosVS");
	pLO		=GD_CreateInputLayout(pGD, DictSZ_GetValueccp(ppDescs, "VPosNormTanTex0"), 4, pCode->mpBytes, pCode->mLen);
	if(pLO == NULL)
	{
		printf("Error creating layout.\n");
		return;
	}
	DictSZ_Addccp(ppLayouts, "VPosNormTanTex0", pLO);
	DictSZ_Addccp(ppLaySizes, "VPosNormTanTex0", 4);
}


static void	sForEachEDesc(const UT_string *pKey, const void *pValue, void *pContext)
{
	D3D11_INPUT_ELEMENT_DESC	*pDesc	=(D3D11_INPUT_ELEMENT_DESC *)pValue;
	if(pDesc == NULL)
	{
		return;
	}

	Match	*pM	=(Match *)pContext;
	if(pM == NULL)
	{
		return;
	}

	int	elCount	=DictSZ_GetValue(pM->mppLayCounts, pKey);

	if(elCount != pM->mElementCount)
	{
		return;
	}

	for(int i=0;i < pM->mElementCount;i++)
	{
		switch(pM->mpElements[i])
		{
			case	EL_POSITION:
			{
				if(strncmp(pDesc[i].SemanticName, "POSITION", 8))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R32G32B32_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_POSITION2:
			{
				if(strncmp(pDesc[i].SemanticName, "POSITION", 8))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R32G32_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_POSITION4:
			{
				if(strncmp(pDesc[i].SemanticName, "POSITION", 8))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R32G32B32A32_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_NORMAL:
			{
				if(strncmp(pDesc[i].SemanticName, "NORMAL", 6))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_COLOR:
			{
				if(strncmp(pDesc[i].SemanticName, "COLOR", 5))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_TEXCOORD:
			{
				if(strncmp(pDesc[i].SemanticName, "TEXCOORD", 8))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R16G16_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_TEXCOORD4:
			{
				if(strncmp(pDesc[i].SemanticName, "TEXCOORD", 8))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_TANGENT:
			{
				if(strncmp(pDesc[i].SemanticName, "TANGENT", 7))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
				{
					return;	//no match
				}
			}
			case	EL_BINDICES:
			{
				if(strncmp(pDesc[i].SemanticName, "BLENDINDICES", 12))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R8G8B8A8_UINT)
				{
					return;	//no match
				}
			}
			case	EL_BWEIGHTS:
			{
				if(strncmp(pDesc[i].SemanticName, "BLENDWEIGHTS", 12))
				{
					return;	//no match
				}
				if(pDesc[i].Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
				{
					return;	//no match
				}
			}
			default:
			{
				assert(false);
			}
		}
	}
	
	//ensure no earlier match
	assert(pM->mpKey == NULL);

	//match!
	pM->mpKey	=pKey;
}


ID3D11InputLayout	*FindMatch(const DictSZ **ppLays, const DictSZ **ppLayCounts,
	const DictSZ **ppDescs, int elems[], int elCount)
{
	Match	m	={	elems, elCount, ppLayCounts, NULL	};

	DictSZ_ForEach(ppDescs, sForEachEDesc, &m);

	if(m.mpKey == NULL)
	{
		return	NULL;	//no match
	}

	return	DictSZ_GetValue(ppLays, m.mpKey);
}