#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<cglm/call.h>
#include	"../UtilityLib/GraphicsDevice.h"

//hold / handle constant buffer related stuffs

//constants that need to match shaders
#define	MAX_BONES			55
#define	NUM_ANI_INTENSITIES	44
#if !defined(SM2)
#define	RADIUS				30
#else
#define	RADIUS				7
#endif
#define	KERNEL_SIZE			(RADIUS * 2 + 1)


//CommonFunctions.hlsli
typedef struct PerFrame_t
{
	mat4		mView;
	mat4		mProjection;
	mat4		mLightViewProj;	//for shadows
	vec3		mEyePos;
	uint32_t	mPadding;		//pad to 16 boundary
}	PerFrame;

//CommonFunctions.hlsli
typedef struct PerObject_t
{
	mat4	mWorld;
	vec4	mSolidColour;
	vec4	mSpecColor;

	//These are considered directional (no falloff)
	vec4	mLightColor0;		//trilights need 3 colors
	vec4	mLightColor1;		//trilights need 3 colors
	vec4	mLightColor2;		//trilights need 3 colors

	vec3	mLightDirection;
	float	mSpecPower;

	//material id for borders etc
	int		mMaterialID;
	vec3	mDanglyForce;
}	PerObject;

//CommonFunctions.hlsli
typedef struct PerShadow_t
{
	vec3	mShadowLightPos;
	bool	mbDirectional;
	float	mShadowAtten;
	vec3	mShadPadding;
}	PerShadow;

//2D.hlsl
typedef struct TwoD_t
{
	vec2	mTextPosition, mSecondLayerOffset;
	vec2	mTextScale;
	vec2	mPadding;
	vec4	mTextColor;
}	TwoD;

//BSP.hlsl
typedef struct BSP_t
{
	bool		mbTextureEnabled;
	vec2		mTexSize;
	uint32_t	mPadding;

	//light style array, make sure size matches HLSL
	//Should be float16 TODO
	float	mpAniIntensities[NUM_ANI_INTENSITIES];
}	BSP;

//post.hlsl
typedef struct Post_t
{
	vec2	mInvViewPort;

	//bloom stuff
	float	mBloomThreshold;
	float	mBloomIntensity;
	float	mBaseIntensity;
	float	mBloomSaturation;
	float	mBaseSaturation;

	//outliner stuff
	float	mTexelSteps;
	float	mThreshold;
	vec2	mScreenSize;

	//bilateral blur stuff
	float	mBlurFallOff;
	float	mSharpNess;
	float	mOpacity;

	//padding
	uint32_t	mPad0, mPad1;

	//gaussianblur stuff, make sure size matches Post.hlsl
	float	mWeightsX[KERNEL_SIZE], mWeightsY[KERNEL_SIZE];
	float	mOffsetsX[KERNEL_SIZE], mOffsetsY[KERNEL_SIZE];
}	Post;

//TextMode.hlsl
typedef struct	TextMode_t
{
	uint32_t	mWidth, mHeight;	//dimensions of screen in pixels
	uint32_t	mCWidth, mCHeight;	//dimensions of screen in character blocks

	//font texture info
	uint32_t	mStartChar;		//first letter of the font bitmap
	uint32_t	mNumColumns;	//number of font columns in the font texture
	uint32_t	mCharWidth;		//width of characters in texels in the font texture (fixed)
	uint32_t	mCharHeight;	//height of characters in texels in the font texture (fixed)
}	TextMode;

typedef struct	CBKeeper_t
{
	//Character.hlsl bone array
	//should match MAX_BONES in CommonFunctions.hlsli
	mat4	mBones[MAX_BONES];

	//GPU side
	ID3D11Buffer	*mpPerObjectBuf;
	ID3D11Buffer	*mpPerFrameBuf;
	ID3D11Buffer	*mpTwoDBuf;
	ID3D11Buffer	*mpCharacterBuf;
	ID3D11Buffer	*mpBSPBuf;
	ID3D11Buffer	*mpPostBuf;
	ID3D11Buffer	*mpPerShadowBuf;
	ID3D11Buffer	*mpTextModeBuf;

	//resource pointers for update
	ID3D11Resource	*mpPerObjectRes;
	ID3D11Resource	*mpPerFrameRes;
	ID3D11Resource	*mpTwoDRes;
	ID3D11Resource	*mpCharacterRes;
	ID3D11Resource	*mpBSPRes;
	ID3D11Resource	*mpPostRes;
	ID3D11Resource	*mpPerShadowRes;
	ID3D11Resource	*mpTextModeRes;

	//CPU side
	PerObject	*mpPerObject;
	PerFrame	*mpPerFrame;
	TwoD		*mpTwoD;
	BSP			*mpBSP;
	Post		*mpPost;
	PerShadow	*mpPerShadow;
	TextMode	*mpTextMode;
}	CBKeeper;


static ID3D11Buffer	*MakeConstantBuffer(GraphicsDevice *pGD, size_t size)
{
	D3D11_BUFFER_DESC	cbDesc;

	//these are kind of odd, but change any one and it breaks		
	cbDesc.BindFlags			=D3D10_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth			=size;
	cbDesc.CPUAccessFlags		=0;	//you'd think write, but nope
	cbDesc.MiscFlags			=0;
	cbDesc.Usage				=0;	//you'd think dynamic here but nope
	cbDesc.StructureByteStride	=0;	//only for structuredbuffer

	//alloc
	return	GraphicsDevice_CreateBuffer(pGD, &cbDesc);
}


CBKeeper	*CBK_Create(GraphicsDevice *pGD)
{
	CBKeeper	*pRet	=malloc(sizeof(CBKeeper));

	//create constant buffers
	pRet->mpPerObjectBuf	=MakeConstantBuffer(pGD, sizeof(PerObject));
	pRet->mpPerFrameBuf		=MakeConstantBuffer(pGD, sizeof(PerFrame));
	pRet->mpTwoDBuf			=MakeConstantBuffer(pGD, sizeof(TwoD));
	pRet->mpBSPBuf			=MakeConstantBuffer(pGD, sizeof(BSP));
	pRet->mpCharacterBuf	=MakeConstantBuffer(pGD, sizeof(mat4) * MAX_BONES);
	pRet->mpPostBuf			=MakeConstantBuffer(pGD, sizeof(Post));
	pRet->mpPerShadowBuf	=MakeConstantBuffer(pGD, sizeof(PerShadow));
	pRet->mpTextModeBuf		=MakeConstantBuffer(pGD, sizeof(TextMode));

	//grab resource pointers for each
	pRet->mpPerObjectBuf->lpVtbl->QueryInterface(pRet->mpPerObjectBuf, &IID_ID3D11Resource, (void **)&pRet->mpPerObjectRes);
	pRet->mpPerFrameBuf->lpVtbl->QueryInterface(pRet->mpPerFrameBuf, &IID_ID3D11Resource, (void **)&pRet->mpPerFrameRes);
	pRet->mpTwoDBuf->lpVtbl->QueryInterface(pRet->mpTwoDBuf, &IID_ID3D11Resource, (void **)&pRet->mpTwoDRes);
	pRet->mpBSPBuf->lpVtbl->QueryInterface(pRet->mpBSPBuf, &IID_ID3D11Resource, (void **)&pRet->mpBSPRes);
	pRet->mpCharacterBuf->lpVtbl->QueryInterface(pRet->mpCharacterBuf, &IID_ID3D11Resource, (void **)&pRet->mpCharacterRes);
	pRet->mpPostBuf->lpVtbl->QueryInterface(pRet->mpPostBuf, &IID_ID3D11Resource, (void **)&pRet->mpPostRes);
	pRet->mpPerShadowBuf->lpVtbl->QueryInterface(pRet->mpPerShadowBuf, &IID_ID3D11Resource, (void **)&pRet->mpPerShadowRes);
	pRet->mpTextModeBuf->lpVtbl->QueryInterface(pRet->mpTextModeBuf, &IID_ID3D11Resource, (void **)&pRet->mpTextModeRes);

	//alloc cpu side data
	pRet->mpPerObject	=malloc(sizeof(PerObject));
	pRet->mpPerFrame	=malloc(sizeof(PerFrame));
	pRet->mpTwoD		=malloc(sizeof(TwoD));
	pRet->mpPost		=malloc(sizeof(Post));
	pRet->mpPerShadow	=malloc(sizeof(PerShadow));
	pRet->mpTextMode	=malloc(sizeof(TextMode));

	return	pRet;
}


void	CBK_SetCommonCBToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//commonfunctions
	GraphicsDevice_VSSetConstantBuffer(pGD, 0, pCBK->mpPerObjectBuf);
	GraphicsDevice_PSSetConstantBuffer(pGD, 0, pCBK->mpPerObjectBuf);
	GraphicsDevice_VSSetConstantBuffer(pGD, 1, pCBK->mpPerFrameBuf);
	GraphicsDevice_PSSetConstantBuffer(pGD, 1, pCBK->mpPerFrameBuf);
}

void	CBK_Set2DCBToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//2d
	GraphicsDevice_VSSetConstantBuffer(pGD, 3, pCBK->mpTwoDBuf);
	GraphicsDevice_PSSetConstantBuffer(pGD, 3, pCBK->mpTwoDBuf);
}

void	CBK_SetCharacterToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//character
	GraphicsDevice_VSSetConstantBuffer(pGD, 4, pCBK->mpCharacterBuf);
}

void	CBK_SetBSPToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//bsp
	GraphicsDevice_VSSetConstantBuffer(pGD, 5, pCBK->mpBSPBuf);
	GraphicsDevice_PSSetConstantBuffer(pGD, 5, pCBK->mpBSPBuf);
}

void	CBK_SetPostToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//post
	GraphicsDevice_VSSetConstantBuffer(pGD, 0, pCBK->mpPostBuf);
	GraphicsDevice_PSSetConstantBuffer(pGD, 0, pCBK->mpPostBuf);
}

void	CBK_SetPerShadowToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//shadows
	GraphicsDevice_VSSetConstantBuffer(pGD, 2, pCBK->mpPerShadowBuf);
	GraphicsDevice_PSSetConstantBuffer(pGD, 2, pCBK->mpPerShadowBuf);
}

void	CBK_SetTextModeToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//textmode
	GraphicsDevice_PSSetConstantBuffer(pGD, 7, pCBK->mpTextModeBuf);
}


void	CBK_UpdateFrame(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpPerFrameRes, pCBK->mpPerFrame);
}

void	CBK_UpdateObject(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpPerObjectRes, pCBK->mpPerObject);
}

void	CBK_UpdateTwoD(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpTwoDRes, pCBK->mpTwoD);
}

void	CBK_UpdateCharacter(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpCharacterRes, pCBK->mBones);
}

void	CBK_UpdateBSP(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpBSPRes, pCBK->mpBSP);
}

void	CBK_UpdatePost(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpPostRes, pCBK->mpPost);
}

void	CBK_UpdatePerShadow(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpPerShadowRes, pCBK->mpPerShadow);
}

void	CBK_UpdateTextMode(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GraphicsDevice_UpdateSubResource(pGD, pCBK->mpTextModeRes, pCBK->mpTextMode);
}


//perframe stuff
void SetView(CBKeeper *pCBK, const mat4 view, const vec3 eyePos)
{
	glmc_mat4_transpose_to(view, pCBK->mpPerFrame->mView);

	//negate into viewspace
	glmc_vec3_flipsign_to(eyePos, pCBK->mpPerFrame->mEyePos);
}


void SetTransposedView(CBKeeper *pCBK, const mat4 view, const vec3 eyePos)
{
	glmc_mat4_copy(view, pCBK->mpPerFrame->mView);

	//negate into viewspace
	glmc_vec3_flipsign_to(eyePos, pCBK->mpPerFrame->mEyePos);
}


void SetTransposedLightViewProj(CBKeeper *pCBK, const mat4 lvp)
{
	glmc_mat4_copy(lvp, pCBK->mpPerFrame->mLightViewProj);
}


void SetLightViewProj(CBKeeper *pCBK, const mat4 lvp)
{
	glmc_mat4_transpose_to(lvp, pCBK->mpPerFrame->mLightViewProj);
}


void SetTransposedProjection(CBKeeper *pCBK, const mat4 proj)
{
	glmc_mat4_copy(proj, pCBK->mpPerFrame->mProjection);
}


void SetProjection(CBKeeper *pCBK, const mat4 proj)
{
	glmc_mat4_transpose_to(proj, pCBK->mpPerFrame->mProjection);
}


void SetTrilights(CBKeeper *pCBK, const vec4 L0, const vec4 L1, const vec4 L2, const vec3 lightDir)
{
	glmc_vec4_copy(L0, pCBK->mpPerObject->mLightColor0);
	glmc_vec4_copy(L1, pCBK->mpPerObject->mLightColor1);
	glmc_vec4_copy(L2, pCBK->mpPerObject->mLightColor2);
	glmc_vec3_copy(lightDir, pCBK->mpPerObject->mLightDirection);
}


void SetTrilights3(CBKeeper *pCBK, const vec3 L0, const vec3 L1, const vec3 L2, const vec3 lightDir)
{
	glmc_vec4_copy3(L0, pCBK->mpPerObject->mLightColor0);
	glmc_vec4_copy3(L1, pCBK->mpPerObject->mLightColor1);
	glmc_vec4_copy3(L2, pCBK->mpPerObject->mLightColor2);

	glmc_vec3_copy(lightDir, pCBK->mpPerObject->mLightDirection);

	pCBK->mpPerObject->mLightColor0[3]	=1.0f;
	pCBK->mpPerObject->mLightColor1[3]	=1.0f;
	pCBK->mpPerObject->mLightColor2[3]	=1.0f;
}


void SetSolidColour(CBKeeper *pCBK, const vec4 sc)
{
	glmc_vec4_copy(sc, pCBK->mpPerObject->mSolidColour);
}


void SetSpecular(CBKeeper *pCBK, const vec4 specColour, float specPow)
{
	glmc_vec4_copy(specColour, pCBK->mpPerObject->mSpecColor);

	pCBK->mpPerObject->mSpecPower	=specPow;
}


void SetSpecularPower(CBKeeper *pCBK, float specPow)
{
	pCBK->mpPerObject->mSpecPower	=specPow;
}


void SetWorldMat(CBKeeper *pCBK, const mat4 world)
{
	glmc_mat4_transpose_to(world, pCBK->mpPerObject->mWorld);
}


void SetTransposedWorldMat(CBKeeper *pCBK, const mat4 world)
{
	glmc_mat4_copy(world, pCBK->mpPerObject->mWorld);
}


void SetMaterialID(CBKeeper *pCBK, int matID)
{
	pCBK->mpPerObject->mMaterialID	=matID;
}


void SetDanglyForce(CBKeeper *pCBK, const vec3 force)
{
	glmc_vec3_copy(force, pCBK->mpPerObject->mDanglyForce);
}


void SetTextTransform(CBKeeper *pCBK, const vec2 textPos, const vec2 textScale)
{
	glmc_vec2_copy(textPos, pCBK->mpTwoD->mTextPosition);
	glmc_vec2_copy(textScale, pCBK->mpTwoD->mTextScale);
}

void SetSecondLayerOffset(CBKeeper *pCBK, const vec2 ofs)
{
	glmc_vec2_copy(ofs, pCBK->mpTwoD->mSecondLayerOffset);
}

void SetTextColor(CBKeeper *pCBK, const vec4 col)
{
	glmc_vec4_copy(col, pCBK->mpTwoD->mTextColor);
}


void SetTextureEnabled(CBKeeper *pCBK, bool bOn)
{
	pCBK->mpBSP->mbTextureEnabled	=bOn;
}


void SetTexSize(CBKeeper *pCBK, const vec2 size)
{
	glmc_vec2_copy(size, pCBK->mpBSP->mTexSize);
}


void SetAniIntensities(CBKeeper *pCBK, const float *pAni)
{
	if(pAni == NULL)
	{
		return;
	}

	memcpy(pCBK->mpBSP->mpAniIntensities, pAni, NUM_ANI_INTENSITIES * sizeof(float));
}


void SetBones(CBKeeper *pCBK, const mat4 *pBones)
{
	memcpy(pCBK->mBones, pBones, MAX_BONES * sizeof(mat4));
}

void SetBonesWithTranspose(CBKeeper *pCBK, const mat4 *pBones)
{
	if(pBones == NULL)
	{
		return;
	}

	for(int i=0;i < MAX_BONES;i++)
	{
		glmc_mat4_transpose_to(pBones[i], pCBK->mBones[i]);
	}
}


void SetInvViewPort(CBKeeper *pCBK, const vec2 port)
{
	glmc_vec2_copy(port, pCBK->mpPost->mInvViewPort);
}


void SetOutlinerVars(CBKeeper *pCBK, const vec2 size, float texelSteps, float threshold)
{
	glmc_vec2_copy(size, pCBK->mpPost->mScreenSize);

	pCBK->mpPost->mTexelSteps	=texelSteps;
	pCBK->mpPost->mThreshold	=threshold;
}


void SetBilateralBlurVars(CBKeeper *pCBK, float fallOff, float sharpness, float opacity)
{
	pCBK->mpPost->mBlurFallOff	=fallOff;
	pCBK->mpPost->mSharpNess	=sharpness;
	pCBK->mpPost->mOpacity		=opacity;
}


void SetBloomVars(CBKeeper *pCBK, float thresh, float intensity,
	float sat, float baseIntensity, float baseSat)
{
	pCBK->mpPost->mBloomThreshold	=thresh;
	pCBK->mpPost->mBloomIntensity	=intensity;
	pCBK->mpPost->mBloomSaturation	=sat;
	pCBK->mpPost->mBaseIntensity	=baseIntensity;
	pCBK->mpPost->mBaseSaturation	=baseSat;
}


void SetWeightsOffsets(CBKeeper *pCBK,
	const float *pWX, const float *pWY,
	const float *pOffx, const float *pOffy)
{
	memcpy(pCBK->mpPost->mWeightsX, pWX, KERNEL_SIZE * sizeof(float));
	memcpy(pCBK->mpPost->mWeightsY, pWY, KERNEL_SIZE * sizeof(float));
	memcpy(pCBK->mpPost->mOffsetsX, pOffx, KERNEL_SIZE * sizeof(float));
	memcpy(pCBK->mpPost->mOffsetsY, pOffy, KERNEL_SIZE * sizeof(float));
}


void SetPerShadow(CBKeeper *pCBK, const vec3 shadowLightPos, bool bDirectional, float shadowAtten)
{
	glmc_vec3_copy(shadowLightPos, pCBK->mpPerShadow->mShadowLightPos);
	pCBK->mpPerShadow->mbDirectional	=bDirectional;
	pCBK->mpPerShadow->mShadowAtten		=shadowAtten;
}


void SetPerShadowDirectional(CBKeeper *pCBK, bool bDirectional)
{
	pCBK->mpPerShadow->mbDirectional	=bDirectional;
}


void SetPerShadowLightPos(CBKeeper *pCBK, const vec3 pos)
{
	glmc_vec3_copy(pos, pCBK->mpPerShadow->mShadowLightPos);
}


void SetTextModeScreenSize(CBKeeper *pCBK, uint32_t width, uint32_t height, uint32_t cwidth, uint32_t cheight)
{
	pCBK->mpTextMode->mWidth	=width;
	pCBK->mpTextMode->mHeight	=height;
	pCBK->mpTextMode->mCWidth	=cwidth;
	pCBK->mpTextMode->mCHeight	=cheight;
}


void SetTextModeFontInfo(CBKeeper *pCBK, uint32_t startChar, uint32_t numColumns, uint32_t charWidth, uint32_t charHeight)
{
	pCBK->mpTextMode->mStartChar	=startChar;
	pCBK->mpTextMode->mNumColumns	=numColumns;
	pCBK->mpTextMode->mCharWidth	=charWidth;
	pCBK->mpTextMode->mCharHeight	=charHeight;
}