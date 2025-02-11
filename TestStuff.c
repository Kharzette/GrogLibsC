#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<assert.h>
#include	<SDL2/SDL.h>
#include	<SDL2/SDL_keycode.h>
#include	<cglm/call.h>
#include	"AudioLib/SoundEffect.h"
#include	"AudioLib/Audio.h"
#include	"MaterialLib/StuffKeeper.h"
#include	"MaterialLib/CBKeeper.h"
#include	"MaterialLib/PostProcess.h"
#include	"MaterialLib/Material.h"
#include	"MaterialLib/MaterialLib.h"
#define CLAY_IMPLEMENTATION
#include	"MaterialLib/UIStuff.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/MiscStuff.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/PrimFactory.h"
#include	"TerrainLib/Terrain.h"
#include	"MeshLib/Mesh.h"
#include	"MeshLib/AnimLib.h"
#include	"MeshLib/Character.h"
#include	"MeshLib/CommonPrims.h"
#include	"InputLib/Input.h"
#include	"UtilityLib/BipedMover.h"
#include	"joltc.h"


#define	RESX			1280
#define	RESY			720
#define	ROT_RATE		10.0f
#define	UVSCALE_RATE	1.0f
#define	KEYTURN_RATE	0.01f
#define	MOVE_RATE		0.1f
#define	HEIGHT_SCALAR	0.15f
#define	RAY_LEN			100.0f
#define	RAY_WIDTH		0.05f
#define	IMPACT_WIDTH	0.2f
#define	NUM_RAYS		1000
#define	MOUSE_TO_ANG	0.001f
#define	MAX_ST_CHARS	256
#define	START_CAM_DIST	5.0f
#define	MAX_CAM_DIST	25.0f
#define	MIN_CAM_DIST	0.25f

//should match CommonFunctions.hlsli
#define	MAX_BONES		55


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	Terrain			*mpTer;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;
	PrimObject		*mpManyImpacts;
	UIStuff			*mpUI;
	BipedMover		*mpBPM;

	//toggles
	bool	mbDrawTerNodes;
	bool	mbMouseLooking;
	bool	mbRunning;
	bool	mbFlyMode;

	

	//character movement
	vec3	mCharMoveVec;

	//misc data
	vec3	mDanglyForce;
	vec3	mLightDir;
	vec3	mEyePos;
	vec3	mPlayerPos;
	float	mDeltaYaw, mDeltaPitch, mCamDist;

}	TestStuff;

//static forward decs
static void		sSetupKeyBinds(Input *pInp);
static void		sSetupRastVP(GraphicsDevice *pGD);
static void		sMoveCharacter(TestStuff *pTS, const vec3 moveVec);
static DictSZ	*sLoadCharacterMeshParts(GraphicsDevice *pGD, StuffKeeper *pSK, const Character *pChar);

//material setups
static Material	*sMakeTerrainMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeNodeBoxesMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK);

//input event handlers
static void	sRandLightEH(void *pContext, const SDL_Event *pEvt);
static void	sDangleDownEH(void *pContext, const SDL_Event *pEvt);
static void	sDangleUpEH(void *pContext, const SDL_Event *pEvt);
static void	sToggleDrawTerNodesEH(void *pContext, const SDL_Event *pEvt);
static void	sToggleFlyModeEH(void *pContext, const SDL_Event *pEvt);
static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveForwardEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveBackEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveLeftEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveRightEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveUpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveDownEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyMoveJumpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeySprintEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnLeftEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnRightEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnUpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnDownEH(void *pContext, const SDL_Event *pEvt);
static void sMouseMoveEH(void *pContext, const SDL_Event *pEvt);
static void sEscEH(void *pContext, const SDL_Event *pEvt);


int main(void)
{
	printf("DirectX action!\n");

	bool	bJPH	=JPH_Init();
	if(!bJPH)
	{
		printf("JPH Init failed.\n");
		return	EXIT_FAILURE;
	}

	Audio	*pAud	=Audio_Create(2);

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	//start in fly mode?
	pTS->mbFlyMode	=true;
	pTS->mCamDist	=START_CAM_DIST;
	
	//set player on corner near origin
	glm_vec3_scale(GLM_VEC3_ONE, 13.0f, pTS->mPlayerPos);

	//input and key / mouse bindings
	Input	*pInp	=INP_CreateInput();
	sSetupKeyBinds(pInp);

	bool	bGDInit	=GD_Init(&pTS->mpGD, "Blortallius!",
				0, 0, RESX, RESY, true, D3D_FEATURE_LEVEL_11_1);
	if(!bGDInit)
	{
		printf("Graphics init failed!\n");
		return	EXIT_FAILURE;
	}

	//turn on border
	GD_SetWindowBordered(pTS->mpGD, true);

	sSetupRastVP(pTS->mpGD);

	StuffKeeper	*pSK	=StuffKeeper_Create(pTS->mpGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pTS->mpGD);
		return	EXIT_FAILURE;
	}

	//a terrain chunk
	pTS->mpTer	=Terrain_Create(pTS->mpGD, "Blort", "Textures/Terrain/HeightMaps/HeightMap.png", 10, HEIGHT_SCALAR);

	//debugdraw quadtree boxes
	int		numBounds;
	vec3	*pMins, *pMaxs;
	Terrain_GetQuadTreeLeafBoxes(pTS->mpTer, &pMins, &pMaxs, &numBounds);

	PrimObject	*pQTBoxes	=PF_CreateCubesFromBoundArray(pMins, pMaxs, numBounds, pTS->mpGD);

	vec4	lightRayCol	={	1.0f, 1.0f, 0.0f, 1.0f	};
	vec4	XAxisCol	={	1.0f, 0.0f, 0.0f, 1.0f	};
	vec4	YAxisCol	={	0.0f, 0.0f, 1.0f, 1.0f	};
	vec4	ZAxisCol	={	0.0f, 1.0f, 0.0f, 1.0f	};

	//test prims
__attribute_maybe_unused__
	PrimObject	*pSkyCube	=PF_CreateCube(10.0f, true, pTS->mpGD);
	LightRay	*pLR		=CP_CreateLightRay(5.0f, 0.25f, pTS->mpGD, pSK);
	AxisXYZ		*pAxis		=CP_CreateAxis(5.0f, 0.1f, pTS->mpGD, pSK);

	CBKeeper	*pCBK	=CBK_Create(pTS->mpGD);
	PostProcess	*pPP	=PP_Create(pTS->mpGD, pSK, pCBK);

	//set sky gradient
	{
		vec3	skyHorizon	={	0.0f, 0.5f, 1.0f	};
		vec3	skyHigh		={	0.0f, 0.25f, 1.0f	};

		CBK_SetSky(pCBK, skyHorizon, skyHigh);
		CBK_SetFogVars(pCBK, 50.0f, 300.0f, true);
	}

	PP_SetTargets(pPP, pTS->mpGD, "BackColor", "BackDepth");

	float	aspect	=(float)RESX / (float)RESY;

	//these need align
	__attribute((aligned(32)))	mat4	charMat, camProj;
	__attribute((aligned(32)))	mat4	textProj, viewMat;

	pTS->mEyePos[1]	=0.6f;
	pTS->mEyePos[2]	=4.5f;

	//game camera
	pTS->mpCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 10.0f);

	//biped mover
	pTS->mpBPM	=BPM_Create(pTS->mpCam);

	BPM_SetMoveMethod(pTS->mpBPM, pTS->mbFlyMode? MOVE_FLY : MOVE_GROUND);

	SoundEffectPlay("synth", pTS->mPlayerPos);

	//3D Projection
	GameCam_GetProjection(pTS->mpCam, camProj);
	CBK_SetProjection(pCBK, camProj);

	//2d projection for text
	glm_ortho(0, RESX, RESY, 0, -1.0f, 1.0f, textProj);

	//set constant buffers to shaders, think I just have to do this once
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	pTS->mLightDir[0]		=0.3f;
	pTS->mLightDir[1]		=-0.7f;
	pTS->mLightDir[2]		=-0.5f;
	pTS->mDanglyForce[0]	=700.0f;

	glm_vec3_normalize(pTS->mLightDir);
	glm_mat4_identity(charMat);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);
	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

	//materials
	Material	*pTerMat	=sMakeTerrainMat(pTS, pSK);
	Material	*pBoxesMat	=sMakeNodeBoxesMat(pTS, pSK);
	Material	*pSkyBoxMat	=sMakeSkyBoxMat(pTS, pSK);

	//character stuffs
	Character	*pChar		=Character_Read("Characters/Protag.Character");
	AnimLib		*pALib		=AnimLib_Read("Characters/Protag.AnimLib");
	MaterialLib	*pCharMats	=MatLib_Read("Characters/Protag.MatLib", pSK);
	DictSZ		*pMeshes	=sLoadCharacterMeshParts(pTS->mpGD, pSK, pChar);

	float	animTime		=0.0f;
	float	maxDT			=0.0f;

	pTS->mbRunning	=true;
	while(pTS->mbRunning)
	{
		pTS->mDeltaYaw		=0.0f;
		pTS->mDeltaPitch	=0.0f;

		UpdateTimer_Stamp(pUT);
		for(float secDelta =UpdateTimer_GetUpdateDeltaSeconds(pUT);
			secDelta > 0.0f;
			secDelta =UpdateTimer_GetUpdateDeltaSeconds(pUT))
		{
			//zero out charmove
			glm_vec3_zero(pTS->mCharMoveVec);

			//do input here
			//move turn etc
			INP_Update(pInp, pTS);

			{
				bool	bJumped	=BPM_Update(pTS->mpBPM, secDelta, pTS->mCharMoveVec);
				if(bJumped)
				{
					SoundEffectPlay("jump", pTS->mPlayerPos);
				}
			}
			sMoveCharacter(pTS, pTS->mCharMoveVec);

			UpdateTimer_UpdateDone(pUT);
		}

		//update materials incase light changed
		MAT_SetLightDirection(pTerMat, pTS->mLightDir);
		MAT_SetLightDirection(pBoxesMat, pTS->mLightDir);

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		//update audio
		Audio_Update(pAud, pTS->mPlayerPos, pTS->mCharMoveVec);

		//player moving?
		float	moving	=glm_vec3_norm(pTS->mCharMoveVec);

		if(moving > 0.0f)
		{
			if(BPM_IsGoodFooting(pTS->mpBPM))
			{
				animTime	+=dt * moving * 200.0f;
			}
			else
			{
				animTime	+=dt * moving * 10.0f;
			}
			AnimLib_Animate(pALib, "LD55ProtagRun", animTime);
		}
		else
		{
			animTime	+=dt;
			AnimLib_Animate(pALib, "LD55ProtagIdle", animTime);
		}

		{
			if(dt > maxDT)
			{
				maxDT	=dt;
			}
		}

		//set no blend, I think post processing turns it on maybe
		GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

		//camera update
		if(pTS->mbFlyMode)
		{
			GameCam_UpdateRotation(pTS->mpCam, pTS->mEyePos, pTS->mDeltaPitch,
									pTS->mDeltaYaw, 0.0f);
		}
		else
		{
			glm_vec3_copy(pTS->mPlayerPos, pTS->mEyePos);
			GameCam_UpdateRotationSecondary(pTS->mpCam, pTS->mPlayerPos, dt,
											pTS->mDeltaPitch, pTS->mDeltaYaw, 0.0f,
											(moving > 0)? true : false);
		}

		//set CB view
		{
			if(pTS->mbFlyMode)
			{
				GameCam_GetViewMatrixFly(pTS->mpCam, viewMat, pTS->mEyePos);
			}
			else
			{
				GameCam_GetViewMatrixThird(pTS->mpCam, viewMat, pTS->mEyePos);
			}

			CBK_SetView(pCBK, viewMat, pTS->mEyePos);

			//set the skybox world mat to match eye pos
			glm_translate_make(viewMat, pTS->mEyePos);
			MAT_SetWorld(pSkyBoxMat, viewMat);
		}


		PP_ClearDepth(pPP, pTS->mpGD, "BackDepth");
		PP_ClearTarget(pPP, pTS->mpGD, "BackColor");

		//update frame CB
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		//turn depth off for sky
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "DisableDepth"));

		//draw sky first
		GD_IASetVertexBuffers(pTS->mpGD, pSkyCube->mpVB, pSkyCube->mVertSize, 0);
		GD_IASetIndexBuffers(pTS->mpGD, pSkyCube->mpIB, DXGI_FORMAT_R16_UINT, 0);

		MAT_Apply(pSkyBoxMat, pCBK, pTS->mpGD);
		GD_DrawIndexed(pTS->mpGD, pSkyCube->mIndexCount, 0, 0);

		//turn depth back on
		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		//draw light ray
		{
			vec3	rayLoc	={	0.0f, 5.0f, 0.0f	};
			CP_DrawLightRay(pLR, pTS->mLightDir, lightRayCol, rayLoc, pCBK, pTS->mpGD);
		}

		//draw xyz axis
		CP_DrawAxis(pAxis, pTS->mLightDir, XAxisCol, YAxisCol, ZAxisCol, pCBK, pTS->mpGD);

		//debug draw quadtree leaf cubes
		if(pTS->mbDrawTerNodes)
		{
			GD_IASetVertexBuffers(pTS->mpGD, pQTBoxes->mpVB, pQTBoxes->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pQTBoxes->mpIB, DXGI_FORMAT_R32_UINT, 0);
			MAT_Apply(pBoxesMat, pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pQTBoxes->mIndexCount, 0, 0);
		}

		//terrain draw
		Terrain_DrawMat(pTS->mpTer, pTS->mpGD, pCBK, pTerMat);

		//set mesh draw stuff
		if(!pTS->mbFlyMode)
		{
			//player direction
			GameCam_GetFlatLookMatrix(pTS->mpCam, charMat);

			//drop mesh to ground
			vec3	feetToCenter	={	0.0f, -0.25f, 0.0f	};

			glm_translate(charMat, feetToCenter);

			Material	*pCM	=MatLib_GetMaterial(pCharMats, "ProtagHell");
			assert(pCM);
			MAT_SetWorld(pCM, charMat);
			MAT_SetDanglyForce(pCM, pTS->mDanglyForce);
		}
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

		Character_Draw(pChar, pMeshes, pCharMats, pALib, pTS->mpGD, pCBK);

		//set proj for 2D
		CBK_SetProjection(pCBK, textProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		//change back to 3D
		CBK_SetProjection(pCBK, camProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		GD_Present(pTS->mpGD);
	}

	GD_Destroy(&pTS->mpGD);

	Audio_Destroy(&pAud);

	return	EXIT_SUCCESS;
}


//event handlers (eh)
static void	sRandLightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	Misc_RandomDirection(pTS->mLightDir);
}

static void	sDangleDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDanglyForce[0]	-=UVSCALE_RATE;
}

static void	sDangleUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDanglyForce[0]	+=UVSCALE_RATE;
}

static void	sToggleDrawTerNodesEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbDrawTerNodes	=!pTS->mbDrawTerNodes;
}

static void	sToggleFlyModeEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbFlyMode	=!pTS->mbFlyMode;

	BPM_SetMoveMethod(pTS->mpBPM, pTS->mbFlyMode? MOVE_FLY : MOVE_GROUND);
}

static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	sRightMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	pTS->mbMouseLooking	=true;
}

static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	SDL_SetRelativeMouseMode(SDL_FALSE);

	pTS->mbMouseLooking	=false;
}

static void	sMouseMoveEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	if(pTS->mbMouseLooking)
	{
		pTS->mDeltaYaw		+=(pEvt->motion.xrel * MOUSE_TO_ANG);
		pTS->mDeltaPitch	+=(pEvt->motion.yrel * MOUSE_TO_ANG);
	}
}

static void	sKeyMoveForwardEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputForward(pTS->mpBPM);
}

static void	sKeyMoveBackEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputBack(pTS->mpBPM);
}

static void	sKeyMoveLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputLeft(pTS->mpBPM);
}

static void	sKeyMoveRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputRight(pTS->mpBPM);
}

static void	sKeyMoveUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputUp(pTS->mpBPM);
}

static void	sKeyMoveDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputDown(pTS->mpBPM);
}

static void	sKeyMoveJumpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputJump(pTS->mpBPM);
}

static void	sKeySprintEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	BPM_InputSprint(pTS->mpBPM, true);
}

static void	sKeyTurnLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	-=KEYTURN_RATE;
}

static void	sKeyTurnRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	+=KEYTURN_RATE;
}

static void	sKeyTurnUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaPitch	+=KEYTURN_RATE;
}

static void	sKeyTurnDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaPitch	-=KEYTURN_RATE;
}

static void	sEscEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbRunning	=false;
}

static Material	*sMakeTerrainMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};

	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosTexFactVS", pSK);
	MAT_SetPShader(pRet, "TriTexFact8PS", pSK);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSRV0(pRet, "Terrain/TerAtlas", pSK);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 3.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	//srv is set in the material, but need input layout set
	Terrain_SetSRVAndLayout(pTS->mpTer, NULL, pSK);

	return	pRet;
}

static Material	*sMakeNodeBoxesMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f		};
	vec3	light1		={	0.5f, 0.5f, 0.5f		};
	vec3	light2		={	0.2f, 0.2f, 0.2f		};
	vec4	ghosty		={	1.0f, 1.0f, 1.0f, 0.5f	};

	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVS", pSK);
	MAT_SetPShader(pRet, "TriSolidSpecPS", pSK);
	MAT_SetSolidColour(pRet, ghosty);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 6.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*sMakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	MAT_SetVShader(pRet, "SkyBoxVS", pSK);
	MAT_SetPShader(pRet, "SkyGradientFogPS", pSK);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static void	sSetupKeyBinds(Input *pInp)
{
	//event style bindings
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_l, sRandLightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_n, sToggleDrawTerNodesEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_f, sToggleFlyModeEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, sEscEH);

	//held bindings
	//movement
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_w, sKeyMoveForwardEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_a, sKeyMoveLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_s, sKeyMoveBackEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_d, sKeyMoveRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_c, sKeyMoveUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_z, sKeyMoveDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_SPACE, sKeyMoveJumpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_LSHIFT, sKeySprintEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_LEFT, sDangleDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_RIGHT, sDangleUpEH);

	//key turning
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_q, sKeyTurnLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_e, sKeyTurnRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_r, sKeyTurnUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_t, sKeyTurnDownEH);

	//move data events
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_MOUSEMOTION, sMouseMoveEH);

	//down/up events
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_RIGHT, sRightMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_RIGHT, sRightMouseUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_LEFT, sLeftMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_LEFT, sLeftMouseUpEH);
}

static void	sSetupRastVP(GraphicsDevice *pGD)
{
	D3D11_RASTERIZER_DESC	rastDesc;
	rastDesc.AntialiasedLineEnable	=false;
	rastDesc.CullMode				=D3D11_CULL_BACK;
	rastDesc.FillMode				=D3D11_FILL_SOLID;
	rastDesc.FrontCounterClockwise	=true;
	rastDesc.MultisampleEnable		=false;
	rastDesc.DepthBias				=0;
	rastDesc.DepthBiasClamp			=0;
	rastDesc.DepthClipEnable		=true;
	rastDesc.ScissorEnable			=false;
	rastDesc.SlopeScaledDepthBias	=0;
	ID3D11RasterizerState	*pRast	=GD_CreateRasterizerState(pGD, &rastDesc);

	D3D11_VIEWPORT	vp;

	vp.Width	=RESX;
	vp.Height	=RESY;
	vp.MaxDepth	=1.0f;
	vp.MinDepth	=0.0f;
	vp.TopLeftX	=0;
	vp.TopLeftY	=0;

	GD_RSSetViewPort(pGD, &vp);
	GD_RSSetState(pGD, pRast);
	GD_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void	sMoveCharacter(TestStuff *pTS, const vec3 moveVec)
{
	if(pTS->mbFlyMode)
	{
		glm_vec3_add(pTS->mEyePos, moveVec, pTS->mEyePos);
	}
	else
	{
		vec3	end, newPos;
		glm_vec3_add(pTS->mPlayerPos, moveVec, end);

		int	footing	=Terrain_MoveSphere(pTS->mpTer, pTS->mPlayerPos, end, 0.25f, newPos);
		
		BPM_SetFooting(pTS->mpBPM, footing);

		//watch for a glitchy move
		float	dist	=glm_vec3_distance(pTS->mPlayerPos, newPos);
		if(dist > 10.0f)
		{
			printf("Glitchy Move: %f %f %f to %f %f %f\n",
				pTS->mPlayerPos[0], pTS->mPlayerPos[1], pTS->mPlayerPos[2], 
				end[0], end[1], end[2]);
		}

		glm_vec3_copy(newPos, pTS->mPlayerPos);
	}
}

static DictSZ *sLoadCharacterMeshParts(GraphicsDevice *pGD, StuffKeeper *pSK, const Character *pChar)
{
	StringList	*pParts	=Character_GetPartList(pChar);

	DictSZ		*pMeshes;
	UT_string	*szMeshPath;

	DictSZ_New(&pMeshes);
	utstring_new(szMeshPath);

	const StringList	*pCur	=SZList_Iterate(pParts);
	while(pCur != NULL)
	{
		utstring_printf(szMeshPath, "Characters/%s.mesh", SZList_IteratorVal(pCur));

		Mesh	*pMesh	=Mesh_Read(pGD, pSK, utstring_body(szMeshPath), false);

		DictSZ_Add(&pMeshes, SZList_IteratorValUT(pCur), pMesh);

		pCur	=SZList_IteratorNext(pCur);
	}

	utstring_done(szMeshPath);
	SZList_Clear(&pParts);

	return	pMeshes;
}
