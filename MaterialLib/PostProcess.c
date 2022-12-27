#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<cglm/call.h>
#include	"StuffKeeper.h"
#include	"../UtilityLib/ListStuff.h"
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../UtilityLib/MiscStuff.h"


//for doing basic filters, but also a linear surface to work on

//gaussianblur stuff from Post.hlsl
#define	RADIUS				30
#define	KERNEL_SIZE			(RADIUS * 2 + 1)
#define	BlurAmount			4f;
#define	MaxOutlineColours	1024;

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
		{	-1, 1, 1	}, {	1, 1, 1	}, {	-1, -1, 1	}, {	1, -1, 1	}
	};

	Misc_Convert2ToF16(0, 0, verts[0].TexCoord0);
	Misc_Convert2ToF16(1, 0, verts[1].TexCoord0);
	Misc_Convert2ToF16(0, 1, verts[2].TexCoord0);
	Misc_Convert2ToF16(1, 1, verts[3].TexCoord0);

	uint16_t	inds[6]	={ 0, 1, 2, 1, 3, 2 };

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


PostProcess	*PP_Create(GraphicsDevice *pGD, StuffKeeper *pSK)
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

}