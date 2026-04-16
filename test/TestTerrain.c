#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<assert.h>
#include	<cglm/call.h>
#include	"AudioLib/SoundEffect.h"
#include	"AudioLib/Audio.h"
#include	"MaterialLib/StuffKeeper.h"
#include	"MaterialLib/CBKeeper.h"
#include	"MaterialLib/PostProcess.h"
#include	"MaterialLib/Material.h"
#include	"MaterialLib/MaterialLib.h"
#define	CLAY_IMPLEMENTATION
#include	"UILib/UIStuff.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/MiscStuff.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/PrimFactory.h"
#include	"UtilityLib/PlaneMath.h"
#include	"TerrainLib/Terrain.h"
#include	"MeshLib/CommonPrims.h"
#include	"InputLib/Input.h"
#include	"PhysicsLib/PhysicsStuff.h"


//macro for supplying fontid and size
#define	FONTDEETS(ui, fid)	.fontId = fid, .fontSize = UI_GetFontSize(ui, fid)

//colors generated from coolors.co or some other generator
//this one is Pastel Dreamland Adventure
Clay_Color	sPastel0	={	0xCD, 0xB4, 0xDB, 0x1F	};
Clay_Color	sPastel1	={	0xFF, 0xC8, 0xDD, 0xFF	};
Clay_Color	sPastel2	={	0xFF, 0xAF, 0xCC, 0xFF	};
Clay_Color	sPastel3	={	0xBD, 0xE0, 0xFE, 0xFF	};
Clay_Color	sPastel4	={	0xA2, 0xD2, 0xFF, 0xFF	};

//Warm Autumn Glow
Clay_Color	sWAG0	={	0x00, 0x30, 0x49, 0xFF	};
Clay_Color	sWAG1	={	0xD6, 0x28, 0x28, 0xFF	};
Clay_Color	sWAG2	={	0xF7, 0x7F, 0x00, 0xFF	};
Clay_Color	sWAG3	={	0xFC, 0xBF, 0x49, 0xFF	};
Clay_Color	sWAG4	={	0xEA, 0xE2, 0xB7, 0xFF	};

#define	RESX				1280
#define	RESY				720
#define	ROT_RATE			10.0f
#define	UVSCALE_RATE		1.0f
#define	MOVE_RATE			0.1f
#define	SPRINT_SCALE		2.0f
#define	KEYTURN_RATE		0.01f
#define	HEIGHT_SCALAR		0.15f
#define	TER_SMOOTH_STEPS	4
#define	RAY_LEN				100.0f
#define	RAY_WIDTH			0.05f
#define	IMPACT_WIDTH		0.2f
#define	NUM_RAYS			1000
#define	MOUSE_TO_ANG		0.001f
#define	SPHERE_SIZE			(1.0f)
#define	MAX_SPHERES			50
#define	BOUNCINESS			(0.8f)
#define	MAX_UI_VERTS		(8192)
#define	RAMP_ANGLE			0.7f	//steepness can traverse on foot
#define	FONT_SIZE_TINY		8
#define	FONT_SIZE_MEDIUM	16
#define	FONT_SIZE_BIG		40



//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	StuffKeeper		*mpSK;
	Terrain			*mpTer;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;
	PrimObject		*mpManyImpacts;
	UIStuff			*mpUI;
	PhysicsStuff	*mpPhys;

	//materials
	Material	*mpTerMat;

	//toggles
	bool	mbMouseLooking;
	bool	mbLeftMouseDown;
	bool	mbRunning;
	bool	mbSprint;
	
	//clay mouse pointer stuff
	Clay_Vector2	mScrollDelta;
	Clay_Vector2	mMousePos;

	//jolt stuff
	uint32_t	mSphereIDs[MAX_SPHERES];
	int			mNumSpheres;
	
	//prims
	PrimObject      *mpSphere;
	
	//misc data
	vec3	mLightDir;
	vec3	mEyePos;
	vec3	mPlayerPos;
	float	mDeltaYaw, mDeltaPitch;

	//font ids for various sizes
	//these will vary as the game assets vary
	uint16_t	mFontIDTiny;
	uint16_t	mFontIDMedium;
	uint16_t	mFontIDBig;

	//selected texture
	const char	*mpSelTexAtlas;

}	TestStuff;

//static forward decs
static void	sSetupKeyBinds(Input *pInp);
static void	sSetupRastVP(GraphicsDevice *pGD);
static void	sMakeSphere(TestStuff *pTS);
static void sCheckLOS(const TestStuff *pTS);
static void	sGetSphereColour(const TestStuff *pTS, int spIdx, vec4 colour);
static void	sSetDefaultCel(GraphicsDevice *pGD, CBKeeper *pCBK);

//clay stuff
static const Clay_RenderCommandArray sCreateLayout(const TestStuff *pTS, float deltaTime);
static void sHandleClayErrors(Clay_ErrorData errorData);

//material setups
static Material	*sMakeTerrainMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeSphereMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*sMakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK);

//input event handlers
static void	sRandLightEH(void *pContext, const SDL_Event *pEvt);
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
static void	sKeySprintOnEH(void *pContext, const SDL_Event *pEvt);
static void	sKeySprintOffEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnLeftEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnRightEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnUpEH(void *pContext, const SDL_Event *pEvt);
static void	sKeyTurnDownEH(void *pContext, const SDL_Event *pEvt);
static void sMouseMoveEH(void *pContext, const SDL_Event *pEvt);
static void sEscEH(void *pContext, const SDL_Event *pEvt);


int main(void)
{
	printf("Terrain related testing...\n");

	PhysicsStuff	*pPhys	=Phys_Create();

//	Audio	*pAud	=Audio_Create(0);

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	pTS->mpPhys		=pPhys;
	
	//set player on corner near origin
	glm_vec3_scale(GLM_VEC3_ONE, 113.0f, pTS->mPlayerPos);

	//input and key / mouse bindings
	Input	*pInp	=INP_CreateInput();
	sSetupKeyBinds(pInp);

	bool	bGDInit	=GD_Init(&pTS->mpGD, "Terrain Test",
				0, 0, RESX, RESY, true, D3D_FEATURE_LEVEL_11_1);
	if(!bGDInit)
	{
		printf("Graphics init failed!\n");
		return	EXIT_FAILURE;
	}

	//turn on border
	GD_SetWindowBordered(pTS->mpGD, true);

	sSetupRastVP(pTS->mpGD);

	pTS->mpSK	=StuffKeeper_Create(pTS->mpGD);
	if(pTS->mpSK == NULL)
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
	LightRay	*pLR		=CP_CreateLightRay(5.0f, 0.25f, pTS->mpGD, pTS->mpSK);
	AxisXYZ		*pAxis		=CP_CreateAxis(5.0f, 0.1f, pTS->mpGD, pTS->mpSK);

	CBKeeper	*pCBK	=CBK_Create(pTS->mpGD);
	PostProcess	*pPP	=PP_Create(pTS->mpGD, pTS->mpSK, pCBK);

	//set sky gradient
	{
		vec3	skyHorizon	={	0.0f, 0.5f, 1.0f	};
		vec3	skyHigh		={	0.0f, 0.25f, 1.0f	};

		CBK_SetSky(pCBK, skyHorizon, skyHigh);
		CBK_SetFogVars(pCBK, 50.0f, 300.0f, true);
	}

	sSetDefaultCel(pTS->mpGD, pCBK);

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

	SoundEffect_Play("synth", pTS->mPlayerPos);

	//3D Projection
	GameCam_GetProjection(pTS->mpCam, camProj);
	CBK_SetProjection(pCBK, camProj);

	//2d projection for text
	glm_ortho(0, RESX, RESY, 0, -1.0f, 1.0f, textProj);

	//set constant buffers to shaders, think I just have to do this once
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	pTS->mpUI	=UI_Create(pTS->mpGD, pTS->mpSK, MAX_UI_VERTS);

	UI_AddAllFonts(pTS->mpUI);

	//pick out 3 useful ids
	pTS->mFontIDTiny	=UI_GetNearestFontSize(pTS->mpUI, FONT_SIZE_TINY);
	pTS->mFontIDMedium	=UI_GetNearestFontSize(pTS->mpUI, FONT_SIZE_MEDIUM);
	pTS->mFontIDBig		=UI_GetNearestFontSize(pTS->mpUI, FONT_SIZE_BIG);

	//clay init
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float)RESX, (float)RESY }, (Clay_ErrorHandler) { sHandleClayErrors });
    Clay_SetMeasureTextFunction(UI_MeasureText, pTS->mpUI);

	Clay_SetDebugModeEnabled(false);

	pTS->mLightDir[0]		=0.3f;
	pTS->mLightDir[1]		=-0.7f;
	pTS->mLightDir[2]		=-0.5f;

	glm_vec3_normalize(pTS->mLightDir);
	glm_mat4_identity(charMat);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);
//	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz
	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 16.66666f);	//60hz

	//materials
	pTS->mpTerMat			=sMakeTerrainMat(pTS, pTS->mpSK);
	Material	*pSphereMat	=sMakeSphereMat(pTS, pTS->mpSK);
	Material	*pSkyBoxMat	=sMakeSkyBoxMat(pTS, pTS->mpSK);

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

			Phys_Update(pPhys, secDelta);

			UpdateTimer_UpdateDone(pUT);
		}

		//update materials incase light changed
		MAT_SetLightDirection(pTS->mpTerMat, pTS->mLightDir);
		MAT_SetLightDirection(pSphereMat, pTS->mLightDir);

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		//update audio
//		Audio_Update(pAud, pTS->mPlayerPos, velocity);

		{
			if(dt > maxDT)
			{
				maxDT	=dt;
			}
		}

		//set no blend, I think post processing turns it on maybe
		GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pTS->mpSK, "NoBlending"));
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pTS->mpSK, "PointWrap"), 0);

		//camera update
		GameCam_UpdateRotation(pTS->mpCam, pTS->mEyePos, pTS->mDeltaPitch,
								pTS->mDeltaYaw, 0.0f);

		//set CB view
		{
			GameCam_GetViewMatrixFly(pTS->mpCam, viewMat, pTS->mEyePos);

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
		GD_OMSetDepthStencilState(pTS->mpGD,
			StuffKeeper_GetDepthStencilState(pTS->mpSK, "DisableDepth"));

		//draw sky first
		GD_IASetVertexBuffers(pTS->mpGD, pSkyCube->mpVB, pSkyCube->mVertSize, 0);
		GD_IASetIndexBuffers(pTS->mpGD, pSkyCube->mpIB, DXGI_FORMAT_R16_UINT, 0);

		MAT_Apply(pSkyBoxMat, pCBK, pTS->mpGD);
		GD_VSSetSRV(pTS->mpGD, pSkyCube->mpVBSRV, 0);
		GD_DrawIndexed(pTS->mpGD, pSkyCube->mIndexCount, 0, 0);

		//turn depth back on
		GD_OMSetDepthStencilState(pTS->mpGD,
			StuffKeeper_GetDepthStencilState(pTS->mpSK, "EnableDepth"));

		//draw light ray
		{
			vec3	rayLoc	={	0.0f, 5.0f, 0.0f	};
			CP_DrawLightRay(pLR, pTS->mLightDir, lightRayCol, rayLoc, pCBK, pTS->mpGD);
		}

		//draw xyz axis
		CP_DrawAxis(pAxis, pTS->mLightDir, XAxisCol, YAxisCol, ZAxisCol, pCBK, pTS->mpGD);

		//terrain draw
		Terrain_DrawMat(pTS->mpTer, pTS->mpGD, pCBK, pTS->mpTerMat);

		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pTS->mpSK, "PointClamp"), 0);

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
			GD_VSSetSRV(pTS->mpGD, pTS->mpSphere->mpVBSRV, 0);
			GD_DrawIndexed(pTS->mpGD, pTS->mpSphere->mIndexCount, 0, 0);
		}

		//set proj for 2D
		CBK_SetProjection(pCBK, textProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		Clay_UpdateScrollContainers(true, pTS->mScrollDelta, dt);

		pTS->mScrollDelta.x	=pTS->mScrollDelta.y	=0.0f;
	
		Clay_RenderCommandArray renderCommands = sCreateLayout(pTS, dt);
	
		UI_BeginDraw(pTS->mpUI);
	
		UI_ClayRender(pTS->mpUI, renderCommands);
	
		UI_EndDraw(pTS->mpUI);
	
		//change back to 3D
		CBK_SetProjection(pCBK, camProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		GD_Present(pTS->mpGD);
	}

	Terrain_Destroy(&pTS->mpTer, pPhys);

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

	GD_SetMouseRelative(pTS->mpGD, true);

	pTS->mbMouseLooking	=true;
}

static void	sRightMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	GD_SetMouseRelative(pTS->mpGD, false);

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

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	if(pTS->mbSprint)
	{
		glm_vec3_scale(forward, SPRINT_SCALE, forward);
	}
	glm_vec3_add(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	sKeyMoveBackEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	if(pTS->mbSprint)
	{
		glm_vec3_scale(forward, SPRINT_SCALE, forward);
	}
	glm_vec3_sub(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	sKeyMoveLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

	if(pTS->mbSprint)
	{
		glm_vec3_scale(right, SPRINT_SCALE, right);
	}
	glm_vec3_sub(pTS->mEyePos, right, pTS->mEyePos);
}

static void	sKeyMoveRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

	if(pTS->mbSprint)
	{
		glm_vec3_scale(right, SPRINT_SCALE, right);
	}
	glm_vec3_add(pTS->mEyePos, right, pTS->mEyePos);
}

static void	sKeyMoveUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	up;
	GameCam_GetUpVec(pTS->mpCam, up);
	glm_vec3_scale(up, MOVE_RATE, up);

	if(pTS->mbSprint)
	{
		glm_vec3_scale(up, SPRINT_SCALE, up);
	}
	glm_vec3_add(pTS->mEyePos, up, pTS->mEyePos);
}

static void	sKeyMoveDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	up;
	GameCam_GetUpVec(pTS->mpCam, up);
	glm_vec3_scale(up, MOVE_RATE, up);

	if(pTS->mbSprint)
	{
		glm_vec3_scale(up, SPRINT_SCALE, up);
	}
	glm_vec3_sub(pTS->mEyePos, up, pTS->mEyePos);
}

static void	sKeySprintOnEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbSprint	=true;
}

static void	sKeySprintOffEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbSprint	=false;
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
	MAT_SetVShader(pRet, "TerrainVS", pSK);
	MAT_SetPShader(pRet, "TerrainPS", pSK);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 3.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);
	if(!MAT_SetSRV0(pRet, "Terrain/TerAtlas", pSK))
	{
		printf("Atlas texture not found!\n");
	}

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
	MAT_SetVShader(pRet, "StaticVS", pSK);
	MAT_SetPShader(pRet, "TriPS", pSK);
	MAT_SetSolidColour(pRet, col);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 16.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static void	sSetDefaultCel(GraphicsDevice *pGD, CBKeeper *pCBK)
{
	float	mins[4]	={	0.0f, 0.3f, 0.6f, 1.0f	};
	float	maxs[4]	={	0.3f, 0.6f, 1.0f, 5.0f	};
	float	snap[4]	={	0.3f, 0.5f, 0.9f, 1.4f	};

	CBK_SetCelSteps(pCBK, mins, maxs, snap, 4);

	CBK_UpdateCel(pCBK, pGD);
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
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_L, sRandLightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, sEscEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_P, sSpawnSphereEH);

	//held bindings
	//movement
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_W, sKeyMoveForwardEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_S, sKeyMoveBackEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_A, sKeyMoveLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_D, sKeyMoveRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_C, sKeyMoveUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_Z, sKeyMoveDownEH);

	//key turning
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_Q, sKeyTurnLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_E, sKeyTurnRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_R, sKeyTurnUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_T, sKeyTurnDownEH);

	//move data events
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_EVENT_MOUSE_MOTION, sMouseMoveEH);

	//down/up events
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_RIGHT, sRightMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_RIGHT, sRightMouseUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_LEFT, sLeftMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_LEFT, sLeftMouseUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDLK_LSHIFT, sKeySprintOnEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDLK_LSHIFT, sKeySprintOffEH);
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

static char	sLOSStrings[MAX_SPHERES][64];

static void sCheckLOS(const TestStuff *pTS)
{
	for(int i=0;i < pTS->mNumSpheres;i++)
	{
		if(Phys_CastRayAtBodyNarrow(pTS->mpPhys, pTS->mEyePos, pTS->mSphereIDs[i]))
		{
			sprintf(sLOSStrings[i], "Can see sphere index %d\n", i);

			vec4	spCol;
			sGetSphereColour(pTS, i, spCol);

			Misc_SRGBToLinear255(spCol, spCol);

			Clay_String	los	={	false, strlen(sLOSStrings[i]), sLOSStrings[i]	};

			CLAY_TEXT(los, CLAY_TEXT_CONFIG({ .fontSize = 26, .textColor = { spCol[0], spCol[1], spCol[2], spCol[3] } }));
		}
	}
}

static void sOnHover(Clay_ElementId eID, Clay_PointerData pnt, void *userData)
{
	//clicked?
	if(pnt.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
	{
		printf("Click! %s\n", eID.stringId.chars);

		if(userData == NULL)
		{
			SoundEffect_Play(eID.stringId.chars, GLM_VEC3_ZERO);
			return;
		}

		TestStuff	*pTS	=(TestStuff *)userData;
		if(pTS == NULL)
		{
			printf("Goofy userdata in hover\n");
			return;
		}

		//change?
		if(pTS->mpSelTexAtlas == NULL ||
			0 != strcmp(pTS->mpSelTexAtlas, eID.stringId.chars))
		{
			MAT_SetSRV0(pTS->mpTerMat, eID.stringId.chars, pTS->mpSK);

			pTS->mpSelTexAtlas	=eID.stringId.chars;
		}
	}
}

static void sFillTexAtlasList(const TestStuff *pTS)
{
	StringList	*pSL	=StuffKeeper_GetTextureList(pTS->mpSK);
	
	const StringList	*pCur	=SZList_Iterate(pSL);

	while(pCur != NULL)
	{
		const char	*pSZTexPath	=SZList_IteratorVal(pCur);

		if(!SZ_StartsWith(pSZTexPath, "Terrain"))
		{
			pCur	=SZList_IteratorNext(pCur);
			continue;
		}
		if(SZ_StartsWith(pSZTexPath, "Terrain/HeightMaps"))
		{
			pCur	=SZList_IteratorNext(pCur);
			continue;
		}

		Clay_String	texStr;

		texStr.chars					=SZList_IteratorVal(pCur);
		texStr.isStaticallyAllocated	=false;
		texStr.length					=strlen(texStr.chars);

		CLAY(Clay__HashString(texStr, 0), { .layout	=
			{
				.layoutDirection = CLAY_LEFT_TO_RIGHT,
				.sizing =
				{
					.width = CLAY_SIZING_FIT(0),
					.height = CLAY_SIZING_FIT(0)
				},
				.padding = { 2, 2, 2, 2 },
				.childGap = 8
			},
			.backgroundColor	=Clay_Hovered()?	sWAG1 : sWAG2,
		})
		{
			Clay_OnHover(sOnHover, pTS);

			CLAY_TEXT(texStr, CLAY_TEXT_CONFIG({
				FONTDEETS(pTS->mpUI, pTS->mFontIDTiny),
				.textColor = sWAG0}));

			pCur	=SZList_IteratorNext(pCur);
		}
	}
}

static void sFillHeightMapList(const TestStuff *pTS)
{
	StringList	*pSL	=StuffKeeper_GetTextureList(pTS->mpSK);
	
	const StringList	*pCur	=SZList_Iterate(pSL);

	while(pCur != NULL)
	{
		const char	*pSZTexPath	=SZList_IteratorVal(pCur);

		if(!SZ_StartsWith(pSZTexPath, "Terrain/HeightMaps"))
		{
			pCur	=SZList_IteratorNext(pCur);
			continue;
		}
		Clay_String	texStr;

		texStr.chars					=SZList_IteratorVal(pCur);
		texStr.isStaticallyAllocated	=false;
		texStr.length					=strlen(texStr.chars);

		CLAY(Clay__HashString(texStr, 0), { .layout	=
			{
				.layoutDirection = CLAY_LEFT_TO_RIGHT,
				.sizing =
				{
					.width = CLAY_SIZING_FIT(0),
					.height = CLAY_SIZING_FIT(0)
				},
				.padding = { 2, 2, 2, 2 },
				.childGap = 8
			},
			.backgroundColor	=Clay_Hovered()?	sWAG1 : sWAG2,
		})
		{
			Clay_OnHover(sOnHover, pTS);

			CLAY_TEXT(texStr, CLAY_TEXT_CONFIG({
				FONTDEETS(pTS->mpUI, pTS->mFontIDTiny),
				.textColor = sWAG0}));

			pCur	=SZList_IteratorNext(pCur);
		}
	}
}

//Stuff to display:
//line of sight indicators to spheres
//list of available heightmaps with selected loaded
//smooth steps, height scalar with buttons to increase / decrease
//list of available texture atlasesesees
//generate a random one?

static Clay_RenderCommandArray	sCreateLayout(const TestStuff *pTS, float deltaTime)
{
	Clay_BeginLayout();


	CLAY(CLAY_ID("OuterContainer"), { .layout =
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
		CLAY(CLAY_ID("LeftPane"), {	.layout	=
			{
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.sizing =
				{
					.width = CLAY_SIZING_PERCENT(0.3f),
					.height = CLAY_SIZING_PERCENT(0.975f)
				},
				.padding = { 8, 8, 8, 8 },
				.childGap = 8
			},
			.backgroundColor	=sPastel0,})
		{
			CLAY(CLAY_ID("TexList"), {
				.layout	=
				{
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing	=
					{
						.width	=CLAY_SIZING_FIT(0),
						.height	=CLAY_SIZING_FIT(0)
					},
					.padding = { 8, 8, 8, 8 },
					.childGap = 8
				},
				.clip	=
				{
					.vertical		=true,
					.childOffset	=Clay_GetScrollOffset()
				},
				.backgroundColor	=sPastel1,
			})
			{
				CLAY_TEXT(CLAY_STRING("Choose a texture atlas:"),
					CLAY_TEXT_CONFIG({
						FONTDEETS(pTS->mpUI, pTS->mFontIDMedium),
						.textColor = sWAG0 }));
				sFillTexAtlasList(pTS);
			}
			CLAY(CLAY_ID("HeightMaps"), {
				.layout	=
				{
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing	=
					{
						.width	=CLAY_SIZING_FIT(0),
						.height	=CLAY_SIZING_FIT(0)
					},
					.padding = { 8, 8, 8, 8 },
					.childGap = 8
				},
				.backgroundColor	=sPastel2,
				.clip	=
				{
					.vertical		=true,
					.childOffset	=Clay_GetScrollOffset()
				}
			})
			{
				CLAY_TEXT(CLAY_STRING("Choose a height map:"),
					CLAY_TEXT_CONFIG({
						FONTDEETS(pTS->mpUI, pTS->mFontIDMedium),
						.textColor = sWAG0 }));
				sFillHeightMapList(pTS);
			}
			CLAY(CLAY_ID("LOS"), {	.layout	=
				{
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing =
					{
						.width = CLAY_SIZING_FIT(0),
						.height = CLAY_SIZING_FIT(0)
					},
					.padding = { 2, 2, 2, 2 },
					.childGap = 2
				},
				.backgroundColor	=sWAG3,})
			{
				CLAY_TEXT(CLAY_STRING("Press P to drop a sphere."),
					CLAY_TEXT_CONFIG({
						FONTDEETS(pTS->mpUI, pTS->mFontIDMedium),
						.textColor = sWAG0 }));
				sCheckLOS(pTS);
			}
		}
	}

	return	Clay_EndLayout(deltaTime);
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