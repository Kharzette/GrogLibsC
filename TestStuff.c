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
#define	CLAY_IMPLEMENTATION
#include	"MaterialLib/UIStuff.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/MiscStuff.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/PrimFactory.h"
#include	"UtilityLib/BipedMover.h"
#include	"UtilityLib/PlaneMath.h"
#include	"TerrainLib/Terrain.h"
#include	"MeshLib/Mesh.h"
#include	"MeshLib/AnimLib.h"
#include	"MeshLib/Character.h"
#include	"MeshLib/CommonPrims.h"
#include	"InputLib/Input.h"
#include	"PhysicsLib/PhysicsStuff.h"


#define	RESX				1280
#define	RESY				720
#define	ROT_RATE			10.0f
#define	UVSCALE_RATE		1.0f
#define	KEYTURN_RATE		0.01f
#define	MOVE_THRESHOLD		0.1f
#define	HEIGHT_SCALAR		0.15f
#define	TER_SMOOTH_STEPS	4
#define	RAY_LEN				100.0f
#define	RAY_WIDTH			0.05f
#define	IMPACT_WIDTH		0.2f
#define	NUM_RAYS			1000
#define	MOUSE_TO_ANG		0.001f
#define	START_CAM_DIST		5.0f
#define	MAX_CAM_DIST		25.0f
#define	MIN_CAM_DIST		0.25f
#define	SPHERE_SIZE			(1.0f)
#define	MAX_SPHERES			50
#define	BOUNCINESS			(0.8f)
#define	PLAYER_RADIUS		(0.25f)
#define	PLAYER_HEIGHT		(1.75f)
#define	PLAYER_EYE_OFFSET	(0.8)
#define	MAX_UI_VERTS		(8192)
#define	RAMP_ANGLE			0.7f	//steepness can traverse on foot

//should match CommonFunctions.hlsli
#define	MAX_BONES	55


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	Terrain			*mpTer;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;
	PrimObject		*mpManyImpacts;
	UIStuff			*mpUI;
	PhysicsStuff	*mpPhys;
	PhysVCharacter	*mpPhysChar;
	BipedMover		*mpBPM;

	//toggles
	bool	mbDrawTerNodes;
	bool	mbMouseLooking;
	bool	mbLeftMouseDown;
	bool	mbRunning;
	bool	mbFlyMode;
	
	//clay pointer stuff
	Clay_Vector2	mScrollDelta;
	Clay_Vector2	mMousePos;

	//jolt stuff
	uint32_t	mSphereIDs[MAX_SPHERES];
	int			mNumSpheres;
	
	//prims
	PrimObject      *mpSphere;
	
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
static void		sMakeSphere(TestStuff *pTS);
static DictSZ	*sLoadCharacterMeshParts(GraphicsDevice *pGD, StuffKeeper *pSK, const Character *pChar);
static void		sFreeCharacterMeshParts(DictSZ **ppMeshes);
static void 	sCheckLOS(const TestStuff *pTS);
static void		sGetSphereColour(const TestStuff *pTS, int spIdx, vec4 colour);

//clay stuff
static const Clay_RenderCommandArray sCreateLayout(const TestStuff *pTS, vec3 velocity);
static void sHandleClayErrors(Clay_ErrorData errorData);

//material setups
static Material	*sMakeTerrainMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeSphereMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeNodeBoxesMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK);

//input event handlers
static void	sRandLightEH(void *pContext, const SDL_Event *pEvt);
static void	sDangleDownEH(void *pContext, const SDL_Event *pEvt);
static void	sDangleUpEH(void *pContext, const SDL_Event *pEvt);
static void	sToggleDrawTerNodesEH(void *pContext, const SDL_Event *pEvt);
static void	sToggleFlyModeEH(void *pContext, const SDL_Event *pEvt);
static void	sSpawnSphereEH(void *pContext, const SDL_Event *pEvt);
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

//extern PhysicsStuff	*_Z11Phys_Createv();

int main(void)
{
	printf("DirectX action!\n");

	PhysicsStuff	*pPhys	=Phys_Create();

//	Audio	*pAud	=Audio_Create(0);

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	//start in fly mode?
	pTS->mbFlyMode	=false;
	pTS->mCamDist	=START_CAM_DIST;
	pTS->mpPhys		=pPhys;
	
	//set player on corner near origin
	glm_vec3_scale(GLM_VEC3_ONE, 113.0f, pTS->mPlayerPos);

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
	pTS->mpTer	=Terrain_Create(pTS->mpGD, pPhys, "Blort",
		"Textures/Terrain/HeightMaps/HeightMap.png",
		TER_SMOOTH_STEPS, HEIGHT_SCALAR);

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
	
	pTS->mpSphere	=PF_CreateSphere(GLM_VEC3_ZERO, SPHERE_SIZE, false, pTS->mpGD);

	//these need align
	__attribute((aligned(32)))	mat4	charMat, camProj;
	__attribute((aligned(32)))	mat4	textProj, viewMat;

	pTS->mEyePos[0] =185.0f;
	pTS->mEyePos[1] =36.0f;
	pTS->mEyePos[2] =180.0f;

	//game camera
	pTS->mpCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 10.0f);

	//biped mover
	pTS->mpBPM	=BPM_Create(pTS->mpCam);

	//physics character
	pTS->mpPhysChar	=Phys_CreateVCharacter(pPhys,
		PLAYER_RADIUS, PLAYER_HEIGHT,
		pTS->mPlayerPos);

	BPM_SetMoveMethod(pTS->mpBPM, pTS->mbFlyMode? BPM_MOVE_FLY : BPM_MOVE_GROUND);

	SoundEffectPlay("synth", pTS->mPlayerPos);

	//3D Projection
	GameCam_GetProjection(pTS->mpCam, camProj);
	CBK_SetProjection(pCBK, camProj);

	//2d projection for text
	glm_ortho(0, RESX, RESY, 0, -1.0f, 1.0f, textProj);

	//set constant buffers to shaders, think I just have to do this once
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	pTS->mpUI	=UI_Create(pTS->mpGD, pSK, MAX_UI_VERTS);

	UI_AddFont(pTS->mpUI, "MeiryoUI26", 0);

	//clay init
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)RESX, (float)RESY }, (Clay_ErrorHandler) { sHandleClayErrors });
    Clay_SetMeasureTextFunction(UI_MeasureText, pTS->mpUI);

	Clay_SetDebugModeEnabled(false);

	pTS->mLightDir[0]		=0.3f;
	pTS->mLightDir[1]		=-0.7f;
	pTS->mLightDir[2]		=-0.5f;
	pTS->mDanglyForce[0]	=700.0f;

	glm_vec3_normalize(pTS->mLightDir);
	glm_mat4_identity(charMat);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);
//	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz
	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 16.66666f);	//60hz

	//materials
	Material	*pTerMat	=sMakeTerrainMat(pTS, pSK);
	Material	*pSphereMat	=sMakeSphereMat(pTS, pSK);
	Material	*pBoxesMat	=sMakeNodeBoxesMat(pTS, pSK);
	Material	*pSkyBoxMat	=sMakeSkyBoxMat(pTS, pSK);

	//character stuffs
	Character	*pChar		=Character_Read("Characters/Protag.Character");
	AnimLib		*pALib		=AnimLib_Read("Characters/ProtagLeft.AnimLib");
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
			//do input here
			//move turn etc
			INP_Update(pInp, pTS);

			{
				vec3	moveVec;

				//check on ground and footing
				bool	bSup		=Phys_VCharacterIsSupported(pTS->mpPhysChar);
				bool	bFooting	=false;
				if(bSup)
				{
					vec3	norm;
					Phys_VCharacterGetGroundNormal(pTS->mpPhysChar, norm);
					bFooting	=PM_IsGroundNormalAng(norm, RAMP_ANGLE);
				}

				bool	bJumped	=BPM_Update(pTS->mpBPM, bSup, bFooting, secDelta, moveVec);
				if(bJumped)
				{
					SoundEffectPlay("jump", pTS->mPlayerPos);
				}

				//moveVec is an amount to move this frame
				//convert to meters per second for phys
				vec3	mpsMove;
				glm_vec3_scale(moveVec, 60, mpsMove);

				vec3	resultVelocity;
				Phys_VCharacterMove(pPhys, pTS->mpPhysChar, mpsMove,
					secDelta, resultVelocity);

				//The biped mover carries a velocity assuming no collisions.
				//If a frame's movement slides along a wall, the velocity
				//will still aim at the wall instead of along it.  This
				//adjusted velocity can be fed back into the biped mover.
				
				bSup	=Phys_VCharacterIsSupported(pTS->mpPhysChar);
				if(bSup)
				{
					//here I think maybe the plane the player is on
					//should be checked for a bad footing situation
					//in which case velocity shouldn't be cleared
					vec3	norm;
					Phys_VCharacterGetGroundNormal(pTS->mpPhysChar, norm);
					if(PM_IsGroundNormalAng(norm, RAMP_ANGLE))
					{
						//if still on the ground, cancel out vertical velocity
//						BPM_AccumulateVelocity(pTS->mpBPM, resultVelocity);
//						BPM_SetVerticalVelocity(pTS->mpBPM, resultVelocity);
						BPM_UpdateWalking2(pTS->mpBPM, false, bJumped, 3, secDelta, resultVelocity);
					}
					else
					{
						//bad footing
						BPM_UpdateWalking2(pTS->mpBPM, true, bJumped, 1.0f, secDelta, resultVelocity);
					}
				}
				else
				{
					//falling
//					BPM_AccumulateVelocity(pTS->mpBPM, resultVelocity);
					BPM_UpdateWalking2(pTS->mpBPM, true, bJumped, 0.1f, secDelta, resultVelocity);
				}
			}

			Phys_VCharacterGetPos(pTS->mpPhysChar, pTS->mPlayerPos);

			Phys_Update(pPhys, secDelta);

			UpdateTimer_UpdateDone(pUT);
		}

		//update materials incase light changed
		MAT_SetLightDirection(pTerMat, pTS->mLightDir);
		MAT_SetLightDirection(pSphereMat, pTS->mLightDir);
		MAT_SetLightDirection(pBoxesMat, pTS->mLightDir);

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		vec3	velocity;
		BPM_GetVelocity(pTS->mpBPM, velocity);

		//update audio
//		Audio_Update(pAud, pTS->mPlayerPos, velocity);

		//player supported?
		bool	bSup	=Phys_VCharacterIsSupported(pTS->mpPhysChar);

		//solid ground?
		vec3	norm;
		Phys_VCharacterGetGroundNormal(pTS->mpPhysChar, norm);
		bool	bFooting	=PM_IsGroundNormalAng(norm, RAMP_ANGLE);

		//player moving?
		float	moving	=glm_vec3_norm(velocity);

		if(moving > MOVE_THRESHOLD)
		{
			if(bSup)
			{
				if(bFooting)
				{
					animTime	+=dt * moving * 1.0f;
					AnimLib_Animate(pALib, "LD55ProtagRun", animTime);
				}
				else
				{
					animTime	+=dt * moving * 0.1f;
					AnimLib_Animate(pALib, "LD55ProtagSlide", animTime);
				}
			}
			else
			{
				animTime	+=dt * moving * 0.1f;
				//todo: fall anim
				AnimLib_Animate(pALib, "LD55ProtagRun", animTime);
			}
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
											(moving > MOVE_THRESHOLD)? true : false);
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

		//terrain draw
		Terrain_DrawMat(pTS->mpTer, pTS->mpGD, pCBK, pTerMat);

		//set mesh draw stuff
		if(!pTS->mbFlyMode)
		{
			//player direction
			GameCam_GetFlatLookMatrix(pTS->mpCam, charMat);

			//drop mesh to ground
			vec3	feetToCenter	={	0.0f, -(PLAYER_HEIGHT * 0.5f), 0.0f	};

			glm_translate(charMat, feetToCenter);

			Material	*pCM	=MatLib_GetMaterial(pCharMats, "ProtagHell");
			assert(pCM);
			MAT_SetWorld(pCM, charMat);
			MAT_SetDanglyForce(pCM, pTS->mDanglyForce);
		}
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

		Character_Draw(pChar, pMeshes, pCharMats, pALib, pTS->mpGD, pCBK);
		
		for(int i=0;i < pTS->mNumSpheres;i++)
		{
			__attribute((aligned(32)))	mat4	wPos;

			vec3	pos;
			Phys_GetBodyPos(pPhys, pTS->mSphereIDs[i], pos);

			vec4	spCol;
			sGetSphereColour(pTS, i, spCol);

			MAT_SetSolidColour(pSphereMat, spCol);

			glm_translate_make(wPos, pos);
			MAT_SetWorld(pSphereMat, wPos);
			
			//draw test spheres (could instance these, but lazy)
			GD_IASetVertexBuffers(pTS->mpGD, pTS->mpSphere->mpVB, pTS->mpSphere->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pTS->mpSphere->mpIB, DXGI_FORMAT_R16_UINT, 0);
			MAT_Apply(pSphereMat, pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pTS->mpSphere->mIndexCount, 0, 0);
		}

		//set proj for 2D
		CBK_SetProjection(pCBK, textProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		Clay_UpdateScrollContainers(true, pTS->mScrollDelta, dt);

		pTS->mScrollDelta.x	=pTS->mScrollDelta.y	=0.0f;
	
		Clay_RenderCommandArray renderCommands = sCreateLayout(pTS, velocity);
	
		UI_BeginDraw(pTS->mpUI);
	
		UI_ClayRender(pTS->mpUI, renderCommands);
	
		UI_EndDraw(pTS->mpUI);
	
		//change back to 3D
		CBK_SetProjection(pCBK, camProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		GD_Present(pTS->mpGD);
	}

	Terrain_Destroy(&pTS->mpTer, pPhys);

	Character_Destroy(pChar);
	sFreeCharacterMeshParts(&pMeshes);

	MatLib_Destroy(&pCharMats);

	for(int i=0;i < pTS->mNumSpheres;i++)
	{
		Phys_RemoveAndDestroyBody(pPhys, pTS->mSphereIDs[i]);
	}

	Phys_Destroy(&pPhys);

	GD_Destroy(&pTS->mpGD);

//	Audio_Destroy(&pAud);

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

	BPM_SetMoveMethod(pTS->mpBPM, pTS->mbFlyMode? BPM_MOVE_FLY : BPM_MOVE_GROUND);
}

static void	sSpawnSphereEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	sMakeSphere(pTS);
}

static void	sLeftMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=true;

	if(!pTS->mbMouseLooking)
	{
		Clay_SetPointerState(pTS->mMousePos, true);
	}
}

static void	sLeftMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbLeftMouseDown	=false;

	if(!pTS->mbMouseLooking)
	{
		Clay_SetPointerState(pTS->mMousePos, false);
	}
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
	else
	{
		pTS->mMousePos.x	=pEvt->motion.x;
		pTS->mMousePos.y	=pEvt->motion.y;

		Clay_SetPointerState(pTS->mMousePos, pTS->mbLeftMouseDown);
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
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 3.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);
	if(!MAT_SetSRV0(pRet, "Terrain/TerAtlas", pSK))
	{
		printf("Atlas texture not found!\n");
	}

	//srv is set in the material, but need input layout set
	Terrain_SetSRVAndLayout(pTS->mpTer, NULL, pSK);

	return	pRet;
}

static Material	*sMakeSphereMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f		};
	vec3	light1		={	0.5f, 0.5f, 0.5f		};
	vec3	light2		={	0.2f, 0.2f, 0.2f		};
	vec4	col			={	1.0f, 1.0f, 1.0f, 1.0f	};

	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVS", pSK);
	MAT_SetPShader(pRet, "TriSolidSpecPS", pSK);
	MAT_SetSolidColour(pRet, col);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 16.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

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
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_p, sSpawnSphereEH);

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

static void	FreeMeshCB(void *pValue)
{
	Mesh	*pMesh	=(Mesh *)pValue;
	if(pMesh == NULL)
	{
		printf("Null mesh in FreeMeshCB!\n");
		return;
	}
	
	Mesh_Destroy(pMesh);
}

static void	sFreeCharacterMeshParts(DictSZ **ppMeshes)
{
	DictSZ_ClearCB(ppMeshes, FreeMeshCB);
}

static void sMakeSphere(TestStuff *pTS)
{
	if(pTS->mNumSpheres >= MAX_SPHERES)
	{
		return; //no moar!
	}
	
	vec3	mins	={	200, 20, 200	};
	vec3	maxs	={	300, 30, 300	};
	vec3	randPoint;

	//randomize layer
	int	layer	=rand();

	//make 2 to 5
	layer	&=0x3;
	layer++;
	
	Misc_RandomPointInBound(mins, maxs, randPoint);
	
	pTS->mSphereIDs[pTS->mNumSpheres]
		=Phys_CreateAndAddSphere(pTS->mpPhys, SPHERE_SIZE, randPoint,
		layer);

	Phys_SetRestitution(pTS->mpPhys, pTS->mSphereIDs[pTS->mNumSpheres], BOUNCINESS);
	
	pTS->mNumSpheres++;
}

static void	sGetSphereColour(const TestStuff *pTS, int spIdx, vec4 colour)
{
	vec4	friend		={	0.5f, 1.0f, 0.5f, 1.0f	};
	vec4	friend_proj	={	1.0f, 1.0f, 0.5f, 1.0f	};
	vec4	enemy		={	1.0f, 0.5f, 0.5f, 1.0f	};
	vec4	enemy_proj	={	1.0f, 0.5f, 1.0f, 1.0f	};
	vec4	unknown		={	0.1f, 0.1f, 0.1f, 1.0f	};

	uint16_t	layer;
	Phys_GetBodyLayer(pTS->mpPhys, pTS->mSphereIDs[spIdx], &layer);

	if(layer == LAY_MOVING_FRIENDLY)
	{
		glm_vec4_copy(friend, colour);
	}
	else if(layer == LAY_MOVING_ENEMY)
	{
		glm_vec4_copy(enemy, colour);
	}
	else if(layer == LAY_MOVING_FRIENDLY_PROJECTILE)
	{
		glm_vec4_copy(friend_proj, colour);
	}
	else if(layer == LAY_MOVING_ENEMY_PROJECTILE)
	{
		glm_vec4_copy(enemy_proj, colour);
	}
	else
	{
		glm_vec4_copy(unknown, colour);
	}
}

static char	sVelString[64];
static char	sLOSStrings[MAX_SPHERES][64];

static void sCheckLOS(const TestStuff *pTS)
{
	vec3	charEyePos;
	vec3	toEye		={0, PLAYER_EYE_OFFSET, 0};

	glm_vec3_add(pTS->mPlayerPos, toEye, charEyePos);

	for(int i=0;i < pTS->mNumSpheres;i++)
	{
		if(Phys_CastRayAtBodyNarrow(pTS->mpPhys, charEyePos, pTS->mSphereIDs[i]))
		{
			sprintf(sLOSStrings[i], "Can see sphere index %d\n", i);

			vec4	spCol;
			sGetSphereColour(pTS, i, spCol);

			Misc_SRGBToLinear255(spCol, spCol);

			Clay_String	los	={	strlen(sLOSStrings[i]), sLOSStrings[i]	};

			CLAY_TEXT(los, CLAY_TEXT_CONFIG({ .fontSize = 26, .textColor = { spCol[0], spCol[1], spCol[2], spCol[3] } }));
		}
	}
}

static Clay_RenderCommandArray	sCreateLayout(const TestStuff *pTS, vec3 velocity)
{
	Clay_BeginLayout();

	sprintf(sVelString, "Velocity: %f, %f, %f", velocity[0], velocity[1], velocity[2]);

	Clay_String	velInfo;

	velInfo.chars	=sVelString;
	velInfo.length	=strlen(sVelString);

	CLAY({.id=CLAY_ID("OuterContainer"), .layout =
		{
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.sizing =
			{
				.width = CLAY_SIZING_GROW(0),
				.height = CLAY_SIZING_GROW(0)
			},
			.padding = { 8, 8, 8, 8 },
			.childGap = 8
		}})
	{
		CLAY_TEXT(velInfo, CLAY_TEXT_CONFIG({ .fontSize = 26, .textColor = {0, 70, 70, 155} }));

		sCheckLOS(pTS);
	}

	return	Clay_EndLayout();
}

static bool reinitializeClay = false;

static void sHandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
    if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxElementCount(Clay_GetMaxElementCount() * 2);
    } else if (errorData.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
        reinitializeClay = true;
        Clay_SetMaxMeasureTextCacheWordCount(Clay_GetMaxMeasureTextCacheWordCount() * 2);
    }
}