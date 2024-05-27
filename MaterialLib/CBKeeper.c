#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"../UtilityLib/GraphicsDevice.h"

//hold / handle constant buffer related stuffs

//shaderlib's constant buffer register usage:
//PerObject		b0
//PerFrame		b1
//PerShadow		b2
//Character		b3
//BSP			b4
//Post			b5
//TextMode		b6
//TwoD			b7
//BSP Dynamic	b8
//BSP Dyn Color	b9
//Cel Shading	b10
#define	PEROBJECT_REG	0
#define	PERFRAME_REG	1
#define	PERSHADOW_REG	2
#define	CHARACTER_REG	3
#define	BSP_REG			4
#define	POST_REG		5
#define	TEXTMODE_REG	6
#define	TWOD_REG		7
#define	BSP_LIGHT_POS	8
#define	BSP_LIGHT_COL	9
#define	CEL_REG			10


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
	vec4		mEyePos;		//w unused
	vec4		mFog;			//start, end, enabled, unused
	vec4		mSkyGradient0;
	vec4		mSkyGradient1;
}	PerFrame;

//CommonFunctions.hlsli
typedef struct PerObject_t
{
	mat4	mWorld;
	vec4	mSolidColour;
	vec4	mSpecColorPow;	//power in w

	//These are considered directional (no falloff)
	//light direction in w component
	vec4	mLightColor0;		//trilights need 3 colors
	vec4	mLightColor1;		//trilights need 3 colors
	vec4	mLightColor2;		//trilights need 3 colors

	//a force vector for doing physicsy stuff
	//integer material id in w
	vec4	mDanglyForceMID;
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

	//pad
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

//Cel.hlsli
typedef struct	CelStuff_t
{
	vec4	mValMin;
	vec4	mValMax;
	vec4	mSnapTo;

	int		mNumSteps;
	vec3	mPad;
}	CelStuff;

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
	ID3D11Buffer	*mpCelBuf;

	//resource pointers for update
	ID3D11Resource	*mpPerObjectRes;
	ID3D11Resource	*mpPerFrameRes;
	ID3D11Resource	*mpTwoDRes;
	ID3D11Resource	*mpCharacterRes;
	ID3D11Resource	*mpBSPRes;
	ID3D11Resource	*mpPostRes;
	ID3D11Resource	*mpPerShadowRes;
	ID3D11Resource	*mpTextModeRes;
	ID3D11Resource	*mpCelRes;

	//CPU side
	PerObject	*mpPerObject;
	PerFrame	*mpPerFrame;
	TwoD		*mpTwoD;
	BSP			*mpBSP;
	Post		*mpPost;
	PerShadow	*mpPerShadow;
	TextMode	*mpTextMode;
	CelStuff	*mpCelStuff;
}	CBKeeper;


static ID3D11Buffer	*MakeConstantBuffer(GraphicsDevice *pGD, size_t size)
{
	D3D11_BUFFER_DESC	cbDesc;

	//constant buffer sizes must be multiples of 16
	assert((size % 16) == 0);

	//these are kind of odd, but change any one and it breaks		
	cbDesc.BindFlags			=D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth			=size;
	cbDesc.CPUAccessFlags		=0;	//you'd think write, but nope
	cbDesc.MiscFlags			=0;
	cbDesc.Usage				=0;	//you'd think dynamic here but nope
	cbDesc.StructureByteStride	=0;	//only for structuredbuffer

	//alloc
	return	GD_CreateBuffer(pGD, &cbDesc);
}


CBKeeper	*CBK_Create(GraphicsDevice *pGD)
{
	CBKeeper	*pRet	=malloc(sizeof(CBKeeper));

	//create constant buffers
	pRet->mpPerObjectBuf	=MakeConstantBuffer(pGD, sizeof(PerObject));
	pRet->mpPerFrameBuf		=MakeConstantBuffer(pGD, sizeof(PerFrame));
	pRet->mpPerShadowBuf	=MakeConstantBuffer(pGD, sizeof(PerShadow));
	pRet->mpCharacterBuf	=MakeConstantBuffer(pGD, sizeof(mat4) * MAX_BONES);
	pRet->mpBSPBuf			=MakeConstantBuffer(pGD, sizeof(BSP));
	pRet->mpPostBuf			=MakeConstantBuffer(pGD, sizeof(Post));
	pRet->mpTextModeBuf		=MakeConstantBuffer(pGD, sizeof(TextMode));
	pRet->mpTwoDBuf			=MakeConstantBuffer(pGD, sizeof(TwoD));
	pRet->mpCelBuf			=MakeConstantBuffer(pGD, sizeof(CelStuff));

	//grab resource pointers for each
	pRet->mpPerObjectBuf->lpVtbl->QueryInterface(pRet->mpPerObjectBuf, &IID_ID3D11Resource, (void **)&pRet->mpPerObjectRes);
	pRet->mpPerFrameBuf->lpVtbl->QueryInterface(pRet->mpPerFrameBuf, &IID_ID3D11Resource, (void **)&pRet->mpPerFrameRes);
	pRet->mpTwoDBuf->lpVtbl->QueryInterface(pRet->mpTwoDBuf, &IID_ID3D11Resource, (void **)&pRet->mpTwoDRes);
	pRet->mpBSPBuf->lpVtbl->QueryInterface(pRet->mpBSPBuf, &IID_ID3D11Resource, (void **)&pRet->mpBSPRes);
	pRet->mpCharacterBuf->lpVtbl->QueryInterface(pRet->mpCharacterBuf, &IID_ID3D11Resource, (void **)&pRet->mpCharacterRes);
	pRet->mpPostBuf->lpVtbl->QueryInterface(pRet->mpPostBuf, &IID_ID3D11Resource, (void **)&pRet->mpPostRes);
	pRet->mpPerShadowBuf->lpVtbl->QueryInterface(pRet->mpPerShadowBuf, &IID_ID3D11Resource, (void **)&pRet->mpPerShadowRes);
	pRet->mpTextModeBuf->lpVtbl->QueryInterface(pRet->mpTextModeBuf, &IID_ID3D11Resource, (void **)&pRet->mpTextModeRes);
	pRet->mpCelBuf->lpVtbl->QueryInterface(pRet->mpCelBuf, &IID_ID3D11Resource, (void **)&pRet->mpCelRes);

	//alloc cpu side data
#ifdef	__AVX__
	pRet->mpPerObject	=aligned_alloc(32, sizeof(PerObject));
	pRet->mpPerFrame	=aligned_alloc(32, sizeof(PerFrame));
	pRet->mpTwoD		=aligned_alloc(32, sizeof(TwoD));
	pRet->mpPost		=aligned_alloc(32, sizeof(Post));
	pRet->mpPerShadow	=aligned_alloc(32, sizeof(PerShadow));
	pRet->mpTextMode	=aligned_alloc(32, sizeof(TextMode));
	pRet->mpCelStuff	=aligned_alloc(32, sizeof(CelStuff));
#else
	pRet->mpPerObject	=aligned_alloc(16, sizeof(PerObject));
	pRet->mpPerFrame	=aligned_alloc(16, sizeof(PerFrame));
	pRet->mpTwoD		=aligned_alloc(16, sizeof(TwoD));
	pRet->mpPost		=aligned_alloc(16, sizeof(Post));
	pRet->mpPerShadow	=aligned_alloc(16, sizeof(PerShadow));
	pRet->mpTextMode	=aligned_alloc(16, sizeof(TextMode));
	pRet->mpCelStuff	=aligned_alloc(16, sizeof(CelStuff));
#endif
	return	pRet;
}


//assign buffers to shaders
void	CBK_SetCommonCBToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//commonfunctions
	GD_VSSetConstantBuffer(pGD, PEROBJECT_REG, pCBK->mpPerObjectBuf);
	GD_PSSetConstantBuffer(pGD, PEROBJECT_REG, pCBK->mpPerObjectBuf);
	GD_VSSetConstantBuffer(pGD, PERFRAME_REG, pCBK->mpPerFrameBuf);
	GD_PSSetConstantBuffer(pGD, PERFRAME_REG, pCBK->mpPerFrameBuf);
	GD_VSSetConstantBuffer(pGD, CEL_REG, pCBK->mpCelBuf);
	GD_PSSetConstantBuffer(pGD, CEL_REG, pCBK->mpCelBuf);
}

void	CBK_Set2DCBToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//2d
	GD_VSSetConstantBuffer(pGD, TWOD_REG, pCBK->mpTwoDBuf);
	GD_PSSetConstantBuffer(pGD, TWOD_REG, pCBK->mpTwoDBuf);
}

void	CBK_SetCharacterToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//character
	GD_VSSetConstantBuffer(pGD, CHARACTER_REG, pCBK->mpCharacterBuf);
}

void	CBK_SetBSPToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//bsp
	GD_VSSetConstantBuffer(pGD, BSP_REG, pCBK->mpBSPBuf);
	GD_PSSetConstantBuffer(pGD, BSP_REG, pCBK->mpBSPBuf);
}

void	CBK_SetPostToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//post
	GD_VSSetConstantBuffer(pGD, POST_REG, pCBK->mpPostBuf);
	GD_PSSetConstantBuffer(pGD, POST_REG, pCBK->mpPostBuf);
}

void	CBK_SetPerShadowToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//shadows
	GD_VSSetConstantBuffer(pGD, PERSHADOW_REG, pCBK->mpPerShadowBuf);
	GD_PSSetConstantBuffer(pGD, PERSHADOW_REG, pCBK->mpPerShadowBuf);
}

void	CBK_SetTextModeToShaders(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	//textmode
	GD_PSSetConstantBuffer(pGD, TEXTMODE_REG, pCBK->mpTextModeBuf);
}


//push changed cpu structs onto gpu
void	CBK_UpdateFrame(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpPerFrameRes, pCBK->mpPerFrame);
}

void	CBK_UpdateObject(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpPerObjectRes, pCBK->mpPerObject);
}

void	CBK_UpdateTwoD(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpTwoDRes, pCBK->mpTwoD);
}

void	CBK_UpdateCharacter(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpCharacterRes, pCBK->mBones);
}

void	CBK_UpdateBSP(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpBSPRes, pCBK->mpBSP);
}

void	CBK_UpdatePost(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpPostRes, pCBK->mpPost);
}

void	CBK_UpdatePerShadow(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpPerShadowRes, pCBK->mpPerShadow);
}

void	CBK_UpdateTextMode(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpTextModeRes, pCBK->mpTextMode);
}

void	CBK_UpdateCel(CBKeeper *pCBK, GraphicsDevice *pGD)
{
	GD_UpdateSubResource(pGD, pCBK->mpCelRes, pCBK->mpCelStuff);
}


//perframe stuff
void CBK_SetView(CBKeeper *pCBK, const mat4 view, const vec3 eyePos)
{
	glm_mat4_transpose_to(view, pCBK->mpPerFrame->mView);

	glm_vec3_copy(eyePos, pCBK->mpPerFrame->mEyePos);
}

void	CBK_SetFogVars(CBKeeper *pCBK, float start, float end, bool bOn)
{
	pCBK->mpPerFrame->mFog[0]	=start;
	pCBK->mpPerFrame->mFog[1]	=end;
	pCBK->mpPerFrame->mFog[2]	=(bOn)?	1.0f : 0.0f;
}

void	CBK_SetSky(CBKeeper *pCBK, const vec3 grad0, const vec3 grad1)
{
	glm_vec3_copy(grad0, pCBK->mpPerFrame->mSkyGradient0);
	glm_vec3_copy(grad1, pCBK->mpPerFrame->mSkyGradient1);
}

void CBK_SetTransposedView(CBKeeper *pCBK, const mat4 view, const vec3 eyePos)
{
	glm_mat4_copy(view, pCBK->mpPerFrame->mView);

	glm_vec3_copy(eyePos, pCBK->mpPerFrame->mEyePos);
}

void CBK_SetTransposedLightViewProj(CBKeeper *pCBK, const mat4 lvp)
{
	glm_mat4_copy(lvp, pCBK->mpPerFrame->mLightViewProj);
}

void CBK_SetLightViewProj(CBKeeper *pCBK, const mat4 lvp)
{
	glm_mat4_transpose_to(lvp, pCBK->mpPerFrame->mLightViewProj);
}

void CBK_SetTransposedProjection(CBKeeper *pCBK, const mat4 proj)
{
	glm_mat4_copy(proj, pCBK->mpPerFrame->mProjection);
}

void CBK_SetProjection(CBKeeper *pCBK, const mat4 proj)
{
	glm_mat4_transpose_to(proj, pCBK->mpPerFrame->mProjection);
}


//per object stuff
//this should have light direction crammed into the w components
void CBK_SetTrilights(CBKeeper *pCBK, const vec4 L0, const vec4 L1, const vec4 L2)
{
	glm_vec4_copy(L0, pCBK->mpPerObject->mLightColor0);
	glm_vec4_copy(L1, pCBK->mpPerObject->mLightColor1);
	glm_vec4_copy(L2, pCBK->mpPerObject->mLightColor2);
}

void CBK_SetTrilights3(CBKeeper *pCBK, const vec3 L0, const vec3 L1, const vec3 L2, const vec3 lightDir)
{
	for(int i=0;i < 3;i++)
	{
		pCBK->mpPerObject->mLightColor0[i]	=L0[i];
		pCBK->mpPerObject->mLightColor1[i]	=L1[i];
		pCBK->mpPerObject->mLightColor2[i]	=L2[i];
	}

	pCBK->mpPerObject->mLightColor0[3]	=lightDir[0];
	pCBK->mpPerObject->mLightColor1[3]	=lightDir[1];
	pCBK->mpPerObject->mLightColor2[3]	=lightDir[2];
}

void CBK_SetSolidColour(CBKeeper *pCBK, const vec4 sc)
{
	glm_vec4_copy(sc, pCBK->mpPerObject->mSolidColour);
}

void CBK_SetSpecular(CBKeeper *pCBK, const vec3 specColour, float specPow)
{
	vec4	spec	={	specColour[0], specColour[1], specColour[2], specPow	};

	glm_vec4_copy(spec, pCBK->mpPerObject->mSpecColorPow);
}

void CBK_SetSpecularPower(CBKeeper *pCBK, float specPow)
{
	pCBK->mpPerObject->mSpecColorPow[4]	=specPow;
}

void CBK_SetWorldMat(CBKeeper *pCBK, const mat4 world)
{
	glm_mat4_transpose_to(world, pCBK->mpPerObject->mWorld);
}

void CBK_SetTransposedWorldMat(CBKeeper *pCBK, const mat4 world)
{
	glm_mat4_copy(world, pCBK->mpPerObject->mWorld);
}

void CBK_SetMaterialID(CBKeeper *pCBK, int matID)
{
	*((int *)&pCBK->mpPerObject->mDanglyForceMID[3])	=matID;
}

void CBK_SetDanglyForce(CBKeeper *pCBK, const vec3 force)
{
	glm_vec3_copy(force, pCBK->mpPerObject->mDanglyForceMID);
}


//2d stuff
void CBK_SetTextTransform(CBKeeper *pCBK, const vec2 textPos, const vec2 textScale)
{
	glm_vec2_copy(textPos, pCBK->mpTwoD->mTextPosition);
	glm_vec2_copy(textScale, pCBK->mpTwoD->mTextScale);
}

void CBK_SetSecondLayerOffset(CBKeeper *pCBK, const vec2 ofs)
{
	glm_vec2_copy(ofs, pCBK->mpTwoD->mSecondLayerOffset);
}

void CBK_SetTextColor(CBKeeper *pCBK, const vec4 col)
{
	glm_vec4_copy(col, pCBK->mpTwoD->mTextColor);
}


//BSP Stuff
void CBK_SetTextureEnabled(CBKeeper *pCBK, bool bOn)
{
	pCBK->mpBSP->mbTextureEnabled	=bOn;
}

void CBK_SetTexSize(CBKeeper *pCBK, const vec2 size)
{
	glm_vec2_copy(size, pCBK->mpBSP->mTexSize);
}

void CBK_SetAniIntensities(CBKeeper *pCBK, const float *pAni)
{
	if(pAni == NULL)
	{
		return;
	}

	memcpy(pCBK->mpBSP->mpAniIntensities, pAni, NUM_ANI_INTENSITIES * sizeof(float));
}


//character bones
void CBK_SetBones(CBKeeper *pCBK, const mat4 *pBones)
{
	memcpy(pCBK->mBones, pBones, MAX_BONES * sizeof(mat4));
}

void CBK_SetBonesWithTranspose(CBKeeper *pCBK, const mat4 *pBones)
{
	if(pBones == NULL)
	{
		return;
	}

	for(int i=0;i < MAX_BONES;i++)
	{
		glm_mat4_transpose_to(pBones[i], pCBK->mBones[i]);
	}
}


//postprocessing
void CBK_SetInvViewPort(CBKeeper *pCBK, const vec2 port)
{
	glm_vec2_copy(port, pCBK->mpPost->mInvViewPort);
}

void CBK_SetOutlinerVars(CBKeeper *pCBK, const vec2 size, float texelSteps, float threshold)
{
	glm_vec2_copy(size, pCBK->mpPost->mScreenSize);

	pCBK->mpPost->mTexelSteps	=texelSteps;
	pCBK->mpPost->mThreshold	=threshold;
}

void CBK_SetBilateralBlurVars(CBKeeper *pCBK, float fallOff, float sharpness, float opacity)
{
	pCBK->mpPost->mBlurFallOff	=fallOff;
	pCBK->mpPost->mSharpNess	=sharpness;
	pCBK->mpPost->mOpacity		=opacity;
}

void CBK_SetBloomVars(CBKeeper *pCBK, float thresh, float intensity,
	float sat, float baseIntensity, float baseSat)
{
	pCBK->mpPost->mBloomThreshold	=thresh;
	pCBK->mpPost->mBloomIntensity	=intensity;
	pCBK->mpPost->mBloomSaturation	=sat;
	pCBK->mpPost->mBaseIntensity	=baseIntensity;
	pCBK->mpPost->mBaseSaturation	=baseSat;
}

void CBK_SetWeightsOffsets(CBKeeper *pCBK,
	const float *pWX, const float *pWY,
	const float *pOffx, const float *pOffy)
{
	memcpy(pCBK->mpPost->mWeightsX, pWX, KERNEL_SIZE * sizeof(float));
	memcpy(pCBK->mpPost->mWeightsY, pWY, KERNEL_SIZE * sizeof(float));
	memcpy(pCBK->mpPost->mOffsetsX, pOffx, KERNEL_SIZE * sizeof(float));
	memcpy(pCBK->mpPost->mOffsetsY, pOffy, KERNEL_SIZE * sizeof(float));
}


//shadows
void CBK_SetPerShadow(CBKeeper *pCBK, const vec3 shadowLightPos, bool bDirectional, float shadowAtten)
{
	glm_vec3_copy(shadowLightPos, pCBK->mpPerShadow->mShadowLightPos);
	pCBK->mpPerShadow->mbDirectional	=bDirectional;
	pCBK->mpPerShadow->mShadowAtten		=shadowAtten;
}

void CBK_SetPerShadowDirectional(CBKeeper *pCBK, bool bDirectional)
{
	pCBK->mpPerShadow->mbDirectional	=bDirectional;
}

void CBK_SetPerShadowLightPos(CBKeeper *pCBK, const vec3 pos)
{
	glm_vec3_copy(pos, pCBK->mpPerShadow->mShadowLightPos);
}


//Text Mode
void CBK_SetTextModeScreenSize(CBKeeper *pCBK, uint32_t width, uint32_t height, uint32_t cwidth, uint32_t cheight)
{
	pCBK->mpTextMode->mWidth	=width;
	pCBK->mpTextMode->mHeight	=height;
	pCBK->mpTextMode->mCWidth	=cwidth;
	pCBK->mpTextMode->mCHeight	=cheight;
}

void CBK_SetTextModeFontInfo(CBKeeper *pCBK, uint32_t startChar, uint32_t numColumns, uint32_t charWidth, uint32_t charHeight)
{
	pCBK->mpTextMode->mStartChar	=startChar;
	pCBK->mpTextMode->mNumColumns	=numColumns;
	pCBK->mpTextMode->mCharWidth	=charWidth;
	pCBK->mpTextMode->mCharHeight	=charHeight;
}


//cel stuff
void	CBK_SetCelSteps(CBKeeper *pCBK, const float *pMins, const float *pMaxs,
						const float *pSteps, int numSteps)
{
	assert(numSteps <= 4);

	pCBK->mpCelStuff->mNumSteps	=numSteps;

	glm_vec4_zero(pCBK->mpCelStuff->mValMin);
	glm_vec4_zero(pCBK->mpCelStuff->mValMax);
	glm_vec4_zero(pCBK->mpCelStuff->mSnapTo);

	for(int i=0;i < numSteps;i++)
	{
		pCBK->mpCelStuff->mValMin[i]	=pMins[i];
		pCBK->mpCelStuff->mValMax[i]	=pMaxs[i];
		pCBK->mpCelStuff->mSnapTo[i]	=pSteps[i];
	}
}