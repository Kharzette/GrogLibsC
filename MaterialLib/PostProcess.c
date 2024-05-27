#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"StuffKeeper.h"
#include	"CBKeeper.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"


//for doing basic filters, but also a linear surface to work on

//gaussianblur stuff from Post.hlsl
#define	RADIUS				30
#define	KERNEL_SIZE			(RADIUS * 2 + 1)
#define	BlurAmount			4.0f
#define	MaxOutlineColours	1024

typedef struct	PostProcess_t
{
	//d3d stuff
	DictSZ	*mpPostTex2Ds;		//ID3D11Texture2D
	DictSZ	*mpPostTargets;		//ID3D11RenderTargetView
	DictSZ	*mpPostDepths;		//ID3D11DepthStencilView
	DictSZ	*mpPostTargSRVs;	//ID3D11ShaderResourceView

	//keep track of lower res rendertargets
	//this info needed when a resize happens
	StringList	*mpHalfResTargets;
	StringList	*mpQuarterResTargets;
	StringList	*mpFixedResTargets;		//don't resize

	//for a fullscreen quad draw
	ID3D11Buffer	*mpQuadVB, *mpQuadIB;

	//stuff
	StuffKeeper	*mpSK;
	int			mResX, mResY;
	vec4		mClearColor;

	//gaussian blur stuff
	float	mSampleWeightsX[KERNEL_SIZE];
	float	mSampleWeightsY[KERNEL_SIZE];
	float	mSampleOffsetsX[KERNEL_SIZE];
	float	mSampleOffsetsY[KERNEL_SIZE];
}	PostProcess;

typedef struct	VPosTex_t
{
	vec3		Position;
	uint16_t	TexCoord0[2];	//16 bit float2
}	VPosTex;


static void MakeQuad(PostProcess *pPP, GraphicsDevice *pGD)
{
	VPosTex	verts[4]	={
		{
			{	-1, 1, 0.9f	}
		},
		{
			{	1, 1, 0.9f	}
		},
		{
			{	-1, -1, 0.9f	}
		},
		{
			{	1, -1, 0.9f	}
		}
	};

	Misc_Convert2ToF16(0, 0, verts[0].TexCoord0);
	Misc_Convert2ToF16(1, 0, verts[1].TexCoord0);
	Misc_Convert2ToF16(0, 1, verts[2].TexCoord0);
	Misc_Convert2ToF16(1, 1, verts[3].TexCoord0);

	uint16_t	inds[6]	={ 2, 1, 0, 2, 3, 1 };

	D3D11_BUFFER_DESC	bDesc;

	bDesc.BindFlags				=D3D11_BIND_VERTEX_BUFFER;
	bDesc.ByteWidth				=sizeof(VPosTex) * 4;
	bDesc.CPUAccessFlags		=0;
	bDesc.MiscFlags				=0;
	bDesc.StructureByteStride	=0;		//only for structuredbuffer stuff
	bDesc.Usage					=D3D11_USAGE_IMMUTABLE;

	pPP->mpQuadVB	=GD_CreateBufferWithData(pGD, &bDesc, &verts, bDesc.ByteWidth);

	bDesc.BindFlags			=D3D11_BIND_INDEX_BUFFER;
	bDesc.ByteWidth			=sizeof(uint16_t) * 6;

	pPP->mpQuadIB	=GD_CreateBufferWithData(pGD, &bDesc, &inds, bDesc.ByteWidth);
}


static float ComputeGaussian(float n)
{
	float	theta	=BlurAmount;
	
	return	(float)((1.0 / sqrtf(2 * GLM_PI * theta)) *
		expf(-(n * n) / (2 * theta * theta)));
}

//from the xna bloom sample
static void InitBlurParams(PostProcess *pPP, float dxX, float dyX, float dxY, float dyY)
{
	int	sampleCountX;
	int	sampleCountY;

	//Doesn't seem to be a way to get array sizes from shaders any more
	//these values need to match KERNEL_SIZE in post.fx
	sampleCountX	=KERNEL_SIZE;
	sampleCountY	=KERNEL_SIZE;
	
	//The first sample always has a zero offset.
	pPP->mSampleWeightsX[0]	=ComputeGaussian(0);
	pPP->mSampleOffsetsX[0]	=0.0f;
	pPP->mSampleWeightsY[0]	=ComputeGaussian(0);
	pPP->mSampleOffsetsY[0]	=0.0f;
	
	//Maintain a sum of all the weighting values.
	float	totalWeightsX	=pPP->mSampleWeightsX[0];
	float	totalWeightsY	=pPP->mSampleWeightsY[0];
	
	//Add pairs of additional sample taps, positioned
	//along a line in both directions from the center.
	for(int i=0;i < sampleCountX / 2; i++)
	{
		//Store weights for the positive and negative taps.
		float	weight				=ComputeGaussian(i + 1);
		pPP->mSampleWeightsX[i * 2 + 1]	=weight;
		pPP->mSampleWeightsX[i * 2 + 2]	=weight;				
		totalWeightsX				+=weight * 2;

		//To get the maximum amount of blurring from a limited number of
		//pixel shader samples, we take advantage of the bilinear filtering
		//hardware inside the texture fetch unit. If we position our texture
		//coordinates exactly halfway between two texels, the filtering unit
		//will average them for us, giving two samples for the price of one.
		//This allows us to step in units of two texels per sample, rather
		//than just one at a time. The 1.5 offset kicks things off by
		//positioning us nicely in between two texels.
		float	sampleOffset	=i * 2 + 1.5f;
		
		vec2	deltaX;
		deltaX[0]	=dxX * sampleOffset;
		deltaX[1]	=dyX * sampleOffset;

		//Store texture coordinate offsets for the positive and negative taps.
		pPP->mSampleOffsetsX[i * 2 + 1]	=deltaX[0];
		pPP->mSampleOffsetsX[i * 2 + 2]	=-deltaX[0];
	}

	//Add pairs of additional sample taps, positioned
	//along a line in both directions from the center.
	for(int i=0;i < sampleCountY / 2; i++)
	{
		//Store weights for the positive and negative taps.
		float	weight				=ComputeGaussian(i + 1);
		pPP->mSampleWeightsY[i * 2 + 1]	=weight;
		pPP->mSampleWeightsY[i * 2 + 2]	=weight;				
		totalWeightsY				+=weight * 2;

		//To get the maximum amount of blurring from a limited number of
		//pixel shader samples, we take advantage of the bilinear filtering
		//hardware inside the texture fetch unit. If we position our texture
		//coordinates exactly halfway between two texels, the filtering unit
		//will average them for us, giving two samples for the price of one.
		//This allows us to step in units of two texels per sample, rather
		//than just one at a time. The 1.5 offset kicks things off by
		//positioning us nicely in between two texels.
		float	sampleOffset	=i * 2 + 1.5f;
		
		vec2	deltaY;
		deltaY[0]	=dxY * sampleOffset;
		deltaY[1]	=dyY * sampleOffset;

		//Store texture coordinate offsets for the positive and negative taps.
		pPP->mSampleOffsetsY[i * 2 + 1]	=deltaY[1];
		pPP->mSampleOffsetsY[i * 2 + 2]	=-deltaY[1];
	}

	//Normalize the list of sample weightings, so they will always sum to one.
	for(int i=0;i < KERNEL_SIZE;i++)
	{
		pPP->mSampleWeightsX[i]	/=totalWeightsX;
	}
	for(int i=0;i < KERNEL_SIZE;i++)
	{
		pPP->mSampleWeightsY[i]	/=totalWeightsY;
	}
}

static void InitPostParams(PostProcess *pPP, CBKeeper *pCBK)
{
	vec2	res, invRes;

	res[0]	=pPP->mResX;
	res[1]	=pPP->mResY;

	invRes[0]	=1.0f / pPP->mResX;
	invRes[1]	=1.0f / pPP->mResY;

	CBK_SetInvViewPort(pCBK, invRes);

	CBK_SetBilateralBlurVars(pCBK, 1.0f, 1.0f, 0.75f);

	CBK_SetBloomVars(pCBK, 0.25f, 1.25f, 1.0f, 1.0f, 1.0f);

	CBK_SetOutlinerVars(pCBK, res, 1.0f, 5.0f);

	InitBlurParams(pPP, 1.0f / (pPP->mResX / 2), 0, 0, 1.0f / (pPP->mResY / 2));

	CBK_SetWeightsOffsets(pCBK, pPP->mSampleWeightsX, pPP->mSampleWeightsY,
		pPP->mSampleOffsetsX, pPP->mSampleOffsetsY);
}


PostProcess	*PP_Create(GraphicsDevice *pGD, StuffKeeper *pSK, CBKeeper *pCBK)
{
	PostProcess	*pPP	=malloc(sizeof(PostProcess));
	memset(pPP, 0, sizeof(PostProcess));

	pPP->mpSK	=pSK;

	pPP->mResX	=GD_GetWidth(pGD);
	pPP->mResY	=GD_GetHeight(pGD);

	//good old xna blue
	pPP->mClearColor[0]	=100.0f / 255.0f;
	pPP->mClearColor[1]	=149.0f / 255.0f;
	pPP->mClearColor[2]	=237.0f / 255.0f;
	pPP->mClearColor[3]	=1.0f;

	//grab targets from GD
	ID3D11RenderTargetView	*pBB	=GD_GetBackBufferView(pGD);
	ID3D11DepthStencilView	*pDS	=GD_GetDepthView(pGD);

	//add a ref for storing them here
	pBB->lpVtbl->AddRef(pBB);
	pDS->lpVtbl->AddRef(pDS);

	//store
	DictSZ_Addccp(&pPP->mpPostTargets, "BackColor", pBB);
	DictSZ_Addccp(&pPP->mpPostDepths, "BackDepth", pDS);

	MakeQuad(pPP, pGD);

	InitPostParams(pPP, pCBK);

	return	pPP;
}


void	PP_MakePostTarget(PostProcess *pPP, GraphicsDevice *pGD,
	const char *szName, int resX, int resY, DXGI_FORMAT fmt)
{
	D3D11_TEXTURE2D_DESC	texDesc;
	memset(&texDesc, 0, sizeof(D3D11_TEXTURE2D_DESC));

	texDesc.ArraySize			=1;
	texDesc.BindFlags			=D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags		=0;
	texDesc.MipLevels			=1;
	texDesc.MiscFlags			=0;
	texDesc.Usage				=D3D11_USAGE_DEFAULT;
	texDesc.Width				=resX;
	texDesc.Height				=resY;
	texDesc.Format				=fmt;
	texDesc.SampleDesc.Count	=1;
	texDesc.SampleDesc.Quality	=0;

	ID3D11Texture2D	*pTex	=GD_CreateTexture(pGD, &texDesc);

	ID3D11RenderTargetView	*pView	=GD_CreateRenderTargetView(pGD, (ID3D11Resource *)pTex, fmt);

	ID3D11ShaderResourceView	*pSRV	=GD_CreateSRV(pGD,  (ID3D11Resource *)pTex, fmt);

	DictSZ_Addccp(&pPP->mpPostTex2Ds, szName, pTex);
	DictSZ_Addccp(&pPP->mpPostTargets, szName, pView);
	DictSZ_Addccp(&pPP->mpPostTargSRVs, szName, pSRV);
}

void	PP_MakePostDepth(PostProcess *pPP, GraphicsDevice *pGD,
	const char *szName, int resX, int resY, DXGI_FORMAT fmt)
{
	D3D11_TEXTURE2D_DESC	texDesc;

	texDesc.ArraySize			=1;
	texDesc.BindFlags			=D3D11_BIND_DEPTH_STENCIL;
	texDesc.CPUAccessFlags		=0;
	texDesc.MipLevels			=1;
	texDesc.MiscFlags			=0;
	texDesc.Usage				=D3D11_USAGE_DEFAULT;
	texDesc.Width				=resX;
	texDesc.Height				=resY;
	texDesc.Format				=fmt;
	texDesc.SampleDesc.Count	=1;
	texDesc.SampleDesc.Quality	=0;

	ID3D11Texture2D	*pTex	=GD_CreateTexture(pGD, &texDesc);

	ID3D11DepthStencilView	*pView	=GD_CreateDepthStencilView(pGD, (ID3D11Resource *)pTex, fmt);

	DictSZ_Addccp(&pPP->mpPostTex2Ds, szName, pTex);
	DictSZ_Addccp(&pPP->mpPostDepths, szName, pView);
}


void PP_ClearTarget(PostProcess *pPP, GraphicsDevice *pGD, const char *szTarg)
{
	ID3D11RenderTargetView	*pTarg	=DictSZ_GetValueccp(pPP->mpPostTargets, szTarg);

	GD_ClearRenderTargetView(pGD, pTarg, pPP->mClearColor);
}

void PP_ClearDepth(PostProcess *pPP, GraphicsDevice *pGD, const char *szDepth)
{
	ID3D11DepthStencilView	*pDepth	=DictSZ_GetValueccp(pPP->mpPostDepths, szDepth);

	GD_ClearDepthStencilView(pGD, pDepth);
}

void PP_SetClearColor(PostProcess *pPP, const vec4 col)
{
	glm_vec4_copy(col, pPP->mClearColor);
}

void PP_SetSRV(PostProcess *pPP, GraphicsDevice *pGD, const char *szName, int slot)
{
	GD_PSSetSRV(pGD, DictSZ_GetValueccp(pPP->mpPostTargSRVs, szName), slot);
}

void PP_SetTargets(PostProcess *pPP, GraphicsDevice *pGD, const char *szTarg, const char *szDepth)
{
	if(szTarg == NULL && szDepth == NULL)
	{
		GD_OMSetRenderTargets(pGD, NULL, NULL);
	}
	else if(strncmp(szTarg, "null", 5) == 0)
	{
		GD_OMSetRenderTargets(pGD, NULL, NULL);
	}
	else if(DictSZ_ContainsKeyccp(pPP->mpPostTargets, szTarg)
		&& DictSZ_ContainsKeyccp(pPP->mpPostDepths, szDepth))
	{
		ID3D11RenderTargetView	*pTarg	=DictSZ_GetValueccp(pPP->mpPostTargets, szTarg);
		ID3D11DepthStencilView	*pDepth	=DictSZ_GetValueccp(pPP->mpPostDepths, szDepth);

		GD_OMSetRenderTargets(pGD, pTarg, pDepth);
	}
	else if(DictSZ_ContainsKeyccp(pPP->mpPostTargets, szTarg)
		&& strncmp(szDepth, "null", 5) == 0)
	{
		ID3D11RenderTargetView	*pTarg	=DictSZ_GetValueccp(pPP->mpPostTargets, szTarg);

		GD_OMSetRenderTargets(pGD, pTarg, NULL);
	}
	else
	{
		//need some sort of error here
		assert(false);
		return;
	}
}


void PP_DrawStage(PostProcess *pPP, GraphicsDevice *pGD, CBKeeper *pCBK)
{	
	GD_IASetVertexBuffers(pGD, pPP->mpQuadVB, sizeof(VPosTex), 0);
	GD_IASetIndexBuffers(pGD, pPP->mpQuadIB, DXGI_FORMAT_R16_UINT, 0);

	GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pPP->mpSK, "VPos"));
	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pPP->mpSK, "SimpleQuadVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pPP->mpSK, "BloomCombinePS"));

	CBK_UpdatePost(pCBK, pGD);

	CBK_SetPostToShaders(pCBK, pGD);

	GD_DrawIndexed(pGD, 6, 0, 0);
}
