#include	<stdint.h>
#include	<utstring.h>
#include	<uthash.h>
#include	<d3d11.h>
#include	<stdio.h>
#include	<string.h>
#include	<assert.h>
#include	"../UtilityLib/GraphicsDevice.h"
#include	"../MaterialLib/StuffKeeper.h"
#include	"../MaterialLib/CBKeeper.h"
#include	"ParticleBoss.h"

#define	VELOCITY_CAP		500.0f
#define	PARTICLE_SIZE		(32 + 32 + 20)	//must match Particles.hlsl
#define	NUM_EMITTER_VALUES	(9)				//must match Particles.hlsl
#define	START_ID			69
#define	THREADX				32				//must match Particles.hlsl

typedef struct	ParticleEmitter_t
{
	//hash key
	uint32_t	mID;

	//d3d data
	ID3D11Buffer	*mpParticleBuf;
	ID3D11Buffer	*mpEmitterValuesBuf;

	//compute shader grinds on these
	ID3D11UnorderedAccessView	*mpPBUAV;
	ID3D11UnorderedAccessView	*mpEVUAV;

	//srv for drawing
	ID3D11ShaderResourceView	*mpPBSRV;

	//texture to draw the particle with
	ID3D11ShaderResourceView	*mpSRV;

	int	mNumVerts, mVertSize;

	//emitter data set to constant buffer
	int	mMaxParticles;

	int		mShape;
	float	mShapeSize;
	vec3	mLineAxis;

	bool	mbOn;
	float	mFrequency;
	float	mTotalSeconds;

	vec3	mPosition;
	float	mStartSize;
	vec4	mStartColor;

	float	mVelocityCap;

	//ranges for random values between min and max
	float	mRotationalVelocityMin, mRotationalVelocityMax;
	float	mVelocityMin, mVelocityMax;
	vec4	mColorVelocityMin, mColorVelocityMax;
	float	mLifeMin, mLifeMax;	
	float	mSizeVelocityMin, mSizeVelocityMax;

	UT_hash_handle	hh;

}	ParticleEmitter;

//changed my mind, compute shader particles!
typedef struct	ParticleBoss_t
{
	GraphicsDevice	*mpGD;
	CBKeeper		*mpCBK;

	//shaders
	ID3D11VertexShader	*mpVS;
	ID3D11PixelShader	*mpPS;
	ID3D11ComputeShader	*mpCSUpdateExisting;
	ID3D11ComputeShader	*mpCSUpdateEmitter;

	//states
	ID3D11DepthStencilState	*mpDepthNoWrite;
	ID3D11BlendState		*mpAlphaBlend;

	//emitters
	ParticleEmitter	*mpEmitters;

	uint32_t	mIDNext;

}	ParticleBoss;


//static forward decs
static void	sFreeEmitter(ParticleEmitter *pEM);
static void	sMakeStructuredBuffer(GraphicsDevice *pGD,
	int structSize, int numItems,
	ID3D11Buffer **ppBuffer, ID3D11ShaderResourceView **ppSRV,
	ID3D11UnorderedAccessView **ppUAV);


ParticleBoss	*PB_Create(GraphicsDevice *pGD,
	const StuffKeeper *pSK, CBKeeper *pCBK)
{
	ParticleBoss	*pRet	=malloc(sizeof(ParticleBoss));

	pRet->mpGD	=pGD;
	pRet->mpCBK	=pCBK;

	pRet->mpVS	=StuffKeeper_GetVertexShader(pSK, "ParticleSBVS");
	pRet->mpPS	=StuffKeeper_GetPixelShader(pSK, "ParticleSBPS");

	pRet->mpCSUpdateExisting	=StuffKeeper_GetComputeShader(pSK, "UpdateExistingParticles");
	pRet->mpCSUpdateEmitter		=StuffKeeper_GetComputeShader(pSK, "UpdateEmitter");

	//states
	pRet->mpDepthNoWrite	=StuffKeeper_GetDepthStencilState(pSK, "DisableDepthWrite");
	pRet->mpAlphaBlend		=StuffKeeper_GetBlendState(pSK, "AlphaBlending");

	pRet->mpEmitters	=NULL;
	pRet->mIDNext		=START_ID;

	return	pRet;
}

//returns particle ID
uint32_t	PB_CreateEmitter(ParticleBoss *pPB, const StuffKeeper *pSK,
	const char *pTexName, uint8_t shape, float startSize, vec4 startColor,
	float emitSec, float shapeSize, vec3 lineAxis, int maxParticles,
	float rotVelMin, float rotVelMax, vec4 colVelMin, vec4 colVelMax,
	float sizeVelMin, float sizeVelMax, float velMin, float velMax,
	float lifeMin, float lifeMax, vec3 pos)
{
	ID3D11ShaderResourceView	*pSRV	=StuffKeeper_GetSRV(pSK, pTexName);
	if(pSRV == NULL)
	{
		return	0;
	}

	ParticleEmitter	*pPE	=malloc(sizeof(ParticleEmitter));
	memset(pPE, 0, sizeof(ParticleEmitter));

	pPE->mID	=pPB->mIDNext;

	pPB->mIDNext++;	//so this one won't be used again

	//add to boss hash
	HASH_ADD_INT(pPB->mpEmitters, mID, pPE);

	//emitter stuff
	pPE->mShape			=shape;
	pPE->mShapeSize		=shapeSize;
	pPE->mbOn			=false;		//start off?
	pPE->mFrequency		=emitSec;
	pPE->mTotalSeconds	=1.0f;
	pPE->mStartSize		=startSize;
	pPE->mVelocityCap	=VELOCITY_CAP;

	pPE->mMaxParticles	=maxParticles;

	//ranges
	pPE->mRotationalVelocityMin	=rotVelMin;
	pPE->mRotationalVelocityMax	=rotVelMax;
	pPE->mVelocityMin			=velMin;
	pPE->mVelocityMax			=velMax;
	pPE->mLifeMin				=lifeMin;
	pPE->mLifeMax				=lifeMax;
	pPE->mSizeVelocityMin		=sizeVelMin;
	pPE->mSizeVelocityMax		=sizeVelMax;

	glm_vec3_copy(lineAxis, pPE->mLineAxis);
	glm_vec3_copy(pos, pPE->mPosition);
	glm_vec4_copy(startColor, pPE->mStartColor);
	glm_vec4_copy(colVelMin, pPE->mColorVelocityMin);
	glm_vec4_copy(colVelMax, pPE->mColorVelocityMax);

	//d3d stuff
	pPE->mpSRV	=pSRV;

	//buffer where particles are created by the compute shader
	sMakeStructuredBuffer(pPB->mpGD, PARTICLE_SIZE, maxParticles,
		&pPE->mpParticleBuf, &pPE->mpPBSRV, &pPE->mpPBUAV);

	//buffer for emitter values that change each update
	D3D11_BUFFER_DESC	bufDesc;
	memset(&bufDesc, 0, sizeof(D3D11_BUFFER_DESC));

	//args buffer
	bufDesc.BindFlags	=D3D11_BIND_UNORDERED_ACCESS;
	bufDesc.ByteWidth	=NUM_EMITTER_VALUES * sizeof(uint32_t);
	bufDesc.MiscFlags	=D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	bufDesc.Usage		=D3D11_USAGE_DEFAULT;

	pPE->mpEmitterValuesBuf	=GD_CreateBuffer(pPB->mpGD, &bufDesc);

	D3D11_UNORDERED_ACCESS_VIEW_DESC	uavDesc;
	memset(&uavDesc, 0, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));

	uavDesc.Format				=DXGI_FORMAT_R32_UINT;
	uavDesc.ViewDimension		=D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements	=NUM_EMITTER_VALUES;

	pPE->mpEVUAV	=GD_CreateUAV(pPB->mpGD,
		(ID3D11Resource *)pPE->mpEmitterValuesBuf, &uavDesc);
	
	return	pPE->mID;
}

void	PB_DestroyEmitter(ParticleBoss *pPB, uint32_t id)
{
	ParticleEmitter	*pEM	=NULL;

	HASH_FIND_INT(pPB->mpEmitters, &id, pEM);

	if(pEM == NULL)
	{
		return;
	}

	//remove from hash table
	HASH_DEL(pPB->mpEmitters, pEM);

	//free emitter
	sFreeEmitter(pEM);
}

void	PB_EmitterActivate(ParticleBoss *pPB, uint32_t emitterID, bool bOn)
{
	ParticleEmitter	*pEM	=NULL;

	HASH_FIND_INT(pPB->mpEmitters, &emitterID, pEM);

	if(pEM == NULL)
	{
		return;
	}

	pEM->mbOn	=bOn;
}

void	PB_SetEmitterPosition(ParticleBoss *pPB, uint32_t emitterID, vec3 pos)
{
	ParticleEmitter	*pEM	=NULL;

	HASH_FIND_INT(pPB->mpEmitters, &emitterID, pEM);

	if(pEM == NULL)
	{
		return;
	}

	glm_vec3_copy(pos, pEM->mPosition);
}

void	PB_Destroy(ParticleBoss **ppPB)
{
	ParticleBoss	*pPB	=*ppPB;

	ParticleEmitter	*pEM, *pTmp;

	//destroy all emitters
	HASH_ITER(hh, pPB->mpEmitters, pEM, pTmp)
	{
		sFreeEmitter(pEM);
	}

	free(pPB);

	*ppPB	=NULL;
}

void	PB_UpdateAndDraw(ParticleBoss *pPB, float secDelta)
{
	for(ParticleEmitter *pEM=pPB->mpEmitters;pEM != NULL;pEM=pEM->hh.next)
	{
		//TODO: might need to fmod this to a reasonable range
		pEM->mTotalSeconds	+=secDelta;

		CBK_SetEmitterColorVMainMax(pPB->mpCBK, pEM->mColorVelocityMin, pEM->mColorVelocityMax);
		CBK_SetEmitterFrequency(pPB->mpCBK, pEM->mFrequency);
		CBK_SetEmitterInts(pPB->mpCBK, pEM->mShape, pEM->mMaxParticles, 0, pEM->mbOn);
		CBK_SetEmitterLifeMinMax(pPB->mpCBK, pEM->mLifeMin, pEM->mLifeMax);
		CBK_SetEmitterLineAxis(pPB->mpCBK, pEM->mLineAxis);
		CBK_SetEmitterPosition(pPB->mpCBK, pEM->mPosition);
		CBK_SetEmitterRotationalVMinMax(pPB->mpCBK, pEM->mRotationalVelocityMin, pEM->mRotationalVelocityMax);
		CBK_SetEmitterShapeSize(pPB->mpCBK, pEM->mShapeSize);
		CBK_SetEmitterSizeVMinMax(pPB->mpCBK, pEM->mSizeVelocityMin, pEM->mSizeVelocityMax);
		CBK_SetEmitterStartColor(pPB->mpCBK, pEM->mStartColor);
		CBK_SetEmitterStartSize(pPB->mpCBK, pEM->mStartSize);
		CBK_SetEmitterVelocityCap(pPB->mpCBK, pEM->mVelocityCap);
		CBK_SetEmitterVMinMax(pPB->mpCBK, pEM->mVelocityMin, pEM->mVelocityMax);
		CBK_SetEmitterSecDelta(pPB->mpCBK, pEM->mTotalSeconds, secDelta);

		CBK_UpdateEmitter(pPB->mpCBK, pPB->mpGD);

		ID3D11UnorderedAccessView	*uavs[]	=
		{	pEM->mpPBUAV, pEM->mpEVUAV	};
		
		UINT	counts[]	={	0, 0	};

		GD_CSSetUnorderedAccessViews(pPB->mpGD, 0, 2, uavs, counts);

		//update existing particles
		GD_CSSetShader(pPB->mpGD, pPB->mpCSUpdateExisting);
		GD_Dispatch(pPB->mpGD, THREADX, 1, 1);

		//this should fill part verts
		GD_CSSetShader(pPB->mpGD, pPB->mpCSUpdateEmitter);
		GD_Dispatch(pPB->mpGD, 1, 1, 1);

		GD_IASetPrimitiveTopology(pPB->mpGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//set blends and such
		GD_OMSetBlendState(pPB->mpGD, pPB->mpAlphaBlend);
		GD_OMSetDepthStencilState(pPB->mpGD, pPB->mpDepthNoWrite);

		GD_IASetVertexBuffers(pPB->mpGD, NULL, 0, 0);
		GD_IASetIndexBuffers(pPB->mpGD, NULL, DXGI_FORMAT_UNKNOWN, 0);
		GD_VSSetShader(pPB->mpGD, pPB->mpVS);
		GD_VSSetSRV(pPB->mpGD, pEM->mpPBSRV, 2);
		GD_PSSetShader(pPB->mpGD, pPB->mpPS);
		GD_PSSetSRV(pPB->mpGD, pEM->mpSRV, 0);

		GD_DrawInstancedIndirect(pPB->mpGD, pEM->mpEmitterValuesBuf, 0);
	}
}

static void	sMakeStructuredBuffer(GraphicsDevice *pGD,
	int structSize, int numItems,
	ID3D11Buffer **ppBuffer, ID3D11ShaderResourceView **ppSRV,
	ID3D11UnorderedAccessView **ppUAV)
{
	//buffer is not optional here
	assert(ppBuffer != NULL);

	D3D11_BUFFER_DESC	bufDesc;
	memset(&bufDesc, 0, sizeof(D3D11_BUFFER_DESC));

	//particle buffer
	bufDesc.BindFlags			=D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufDesc.ByteWidth			=structSize * numItems;
	bufDesc.CPUAccessFlags		=0;
	bufDesc.MiscFlags			=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufDesc.StructureByteStride	=structSize;
	bufDesc.Usage				=D3D11_USAGE_DEFAULT;

	*ppBuffer	=GD_CreateBuffer(pGD, &bufDesc);

	if(ppSRV != NULL)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC	srvDesc;
		memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		srvDesc.Format				=DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension		=D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth	=numItems;

		*ppSRV	=GD_CreateSRV(pGD, (ID3D11Resource *)*ppBuffer, &srvDesc);
	}

	if(ppUAV != NULL)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC	uavDesc;
		memset(&uavDesc, 0, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));

		uavDesc.Format				=DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension		=D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements	=numItems;

		*ppUAV	=GD_CreateUAV(pGD, (ID3D11Resource *)*ppBuffer, &uavDesc);
	}
}

static void	sFreeEmitter(ParticleEmitter *pEM)
{
	//free gpu stuff
	//UAV
	pEM->mpPBUAV->lpVtbl->Release(pEM->mpPBUAV);
	pEM->mpEVUAV->lpVtbl->Release(pEM->mpEVUAV);

	//SRVs
	pEM->mpPBSRV->lpVtbl->Release(pEM->mpPBSRV);

	//buffers
	pEM->mpParticleBuf->lpVtbl->Release(pEM->mpParticleBuf);
	pEM->mpEmitterValuesBuf->lpVtbl->Release(pEM->mpEmitterValuesBuf);

	free(pEM);
}