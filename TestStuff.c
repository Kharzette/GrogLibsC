#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<assert.h>
#include	<x86intrin.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_keycode.h>
#include	<cglm/call.h>
//#include	"AudioLib/Audio.h"	audio stuff not ready yet
#include	"MaterialLib/StuffKeeper.h"
#include	"MaterialLib/CBKeeper.h"
#include	"MaterialLib/PostProcess.h"
#include	"MaterialLib/Material.h"
#include	"MaterialLib/ScreenText.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/MiscStuff.h"
#include	"UtilityLib/ConvexVolume.h"
#include	"UtilityLib/GameCamera.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/PrimFactory.h"
#include	"UtilityLib/PlaneMath.h"
#include	"TerrainLib/Terrain.h"
#include	"MeshLib/Mesh.h"
#include	"MeshLib/AnimLib.h"
#include	"MeshLib/Character.h"
#include	"MeshLib/CommonPrims.h"
#include	"InputLib/Input.h"


#define	RESX			800
#define	RESY			600
#define	ROT_RATE		10.0f
#define	UVSCALE_RATE	1.0f
#define	KEYTURN_RATE	0.01f
#define	MOVE_RATE		0.1f
#define	HEIGHT_SCALAR	0.5f
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

//data to store test rays
static vec3	sRayStarts[NUM_RAYS];
static vec3	sRayEnds[NUM_RAYS];
static vec4	sRayResults[NUM_RAYS];
static vec3	sRayImpacts[NUM_RAYS];
static int	sNumRayImpacts;

//test box
static vec3	sTestBoxMin	={	-0.25f, -0.85f, -0.25f	};
static vec3	sTestBoxMax	={	0.25f, 0.85f, 0.25f		};

//test triangle
static vec3	sTestTri[3]	={	{5, 5, 5}, {9, 5, 5}, {9, 5, 9}	};


//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	Terrain			*mpTer;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;
	PrimObject		*mpManyImpacts;
	ScreenText		*mpST;

	//toggles
	bool	mbDrawTerNodes;
	bool	mbDrawManyRays;
	bool	mbDrawManyImpacts;
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

	//single ray cast vars
	int		mDrawHit;
	vec3	mHitPos;
	vec4	mHitPlane;
}	TestStuff;

//static forward decs
static void	TestManyRays(const Terrain *pTer);
static void	PrintRandomPointInTerrain(const Terrain *pTer);
static int	TestOneRay(const Terrain *pTer, const GameCamera *pCam, const vec3 eyePos, vec3 hitPos, vec4 hitPlane);
static int	TestOneMove(const Terrain *pTer, vec3 hitPos, vec4 hitPlane);
static void	ReBuildManyRayPrims(PrimObject **ppMR, PrimObject **ppMI, GraphicsDevice *pGD);
static void	SetupKeyBinds(Input *pInp);
static void	SetupRastVP(GraphicsDevice *pGD);
static void SetupDebugStrings(TestStuff *pTS, const StuffKeeper *pSK);

//material setups
static Material	*MakeSphereMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeCubeMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeRaysMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeHitsMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeCharacterMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeTerrainMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeNodeBoxesMat(TestStuff *pTS, const StuffKeeper *pSK);
static Material	*MakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK);

//input event handlers
static void	RandLightEH(void *pContext, const SDL_Event *pEvt);
static void	DangleDownEH(void *pContext, const SDL_Event *pEvt);
static void	DangleUpEH(void *pContext, const SDL_Event *pEvt);
static void	CastOneRayEH(void *pContext, const SDL_Event *pEvt);
static void	PrintRandomEH(void *pContext, const SDL_Event *pEvt);
static void	CastManyRaysEH(void *pContext, const SDL_Event *pEvt);
static void	ToggleDrawTerNodesEH(void *pContext, const SDL_Event *pEvt);
static void	ToggleDrawManyRaysEH(void *pContext, const SDL_Event *pEvt);
static void	ToggleDrawManyImpactsEH(void *pContext, const SDL_Event *pEvt);
static void	ToggleFlyModeEH(void *pContext, const SDL_Event *pEvt);
static void	LeftMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	LeftMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	RightMouseDownEH(void *pContext, const SDL_Event *pEvt);
static void	RightMouseUpEH(void *pContext, const SDL_Event *pEvt);
static void	KeyMoveForwardEH(void *pContext, const SDL_Event *pEvt);
static void	KeyMoveBackEH(void *pContext, const SDL_Event *pEvt);
static void	KeyMoveLeftEH(void *pContext, const SDL_Event *pEvt);
static void	KeyMoveRightEH(void *pContext, const SDL_Event *pEvt);
static void	KeyMoveUpEH(void *pContext, const SDL_Event *pEvt);
static void	KeyMoveDownEH(void *pContext, const SDL_Event *pEvt);
static void	KeyTurnLeftEH(void *pContext, const SDL_Event *pEvt);
static void	KeyTurnRightEH(void *pContext, const SDL_Event *pEvt);
static void	KeyTurnUpEH(void *pContext, const SDL_Event *pEvt);
static void	KeyTurnDownEH(void *pContext, const SDL_Event *pEvt);
static void MouseMoveEH(void *pContext, const SDL_Event *pEvt);
static void EscEH(void *pContext, const SDL_Event *pEvt);


int main(void)
{
	printf("DirectX on looney loonix!\n");

	//store a bunch of vars in a struct
	//for ref/modifying by input handlers
	TestStuff	*pTS	=malloc(sizeof(TestStuff));
	memset(pTS, 0, sizeof(TestStuff));

	//start in fly mode
	pTS->mbFlyMode	=true;
	pTS->mCamDist	=START_CAM_DIST;
	
	//set player on corner near origin
	glm_vec3_scale(GLM_VEC3_ONE, 3.0f, pTS->mPlayerPos);

	//input and key / mouse bindings
	Input	*pInp	=INP_CreateInput();
	SetupKeyBinds(pInp);

	GD_Init(&pTS->mpGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_0);

	SetupRastVP(pTS->mpGD);

	StuffKeeper	*pSK	=StuffKeeper_Create(pTS->mpGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pTS->mpGD);
		return	EXIT_FAILURE;
	}

	//a terrain chunk
	pTS->mpTer	=Terrain_Create(pTS->mpGD, "Blort", "Textures/Terrain/HeightMaps/MZCloud.png", 10, HEIGHT_SCALAR);

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
	PrimObject	*pSphere	=PF_CreateSphere(GLM_VEC3_ZERO, 0.25f, pTS->mpGD);
	PrimObject	*pCube		=PF_CreateCubeFromBounds(sTestBoxMin, sTestBoxMax, pTS->mpGD);
	PrimObject	*pSkyCube	=PF_CreateCube(10.0f, true, pTS->mpGD);
	PrimObject	*pPOTri		=PF_CreateTri(sTestTri, pTS->mpGD);
	LightRay	*pLR		=CP_CreateLightRay(5.0f, 0.25f, pTS->mpGD, pSK);
	AxisXYZ		*pAxis		=CP_CreateAxis(5.0f, 0.1f, pTS->mpGD, pSK);

	CBKeeper	*pCBK	=CBK_Create(pTS->mpGD);
	PostProcess	*pPP	=PP_Create(pTS->mpGD, pSK, pCBK);

	SetupDebugStrings(pTS, pSK);

	//set sky gradient
	{
		vec3	skyHorizon	={	0.0f, 0.5f, 1.0f	};
		vec3	skyHigh		={	0.0f, 0.25f, 1.0f	};

		CBK_SetSky(pCBK, skyHorizon, skyHigh);
		CBK_SetFogVars(pCBK, 50.0f, 300.0f, true);
	}

	PP_MakePostTarget(pPP, pTS->mpGD, "LinearColor", RESX, RESY, DXGI_FORMAT_R8G8B8A8_UNORM);
	PP_MakePostDepth(pPP, pTS->mpGD, "LinearDepth", RESX, RESY, DXGI_FORMAT_D32_FLOAT);

	float	aspect	=(float)RESX / (float)RESY;

	mat4	charMat, hitSphereMat;

	pTS->mEyePos[1]	=0.6f;
	pTS->mEyePos[2]	=4.5f;

	pTS->mpCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 10.0f);

	//3D Projection
	mat4	camProj;
	GameCam_GetProjection(pTS->mpCam, camProj);
	CBK_SetProjection(pCBK, camProj);

	//2d projection for text
	mat4	textProj;
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
	Material	*pSphereMat	=MakeSphereMat(pTS, pSK);
	Material	*pCubeMat	=MakeCubeMat(pTS, pSK);
	Material	*pRaysMat	=MakeRaysMat(pTS, pSK);
	Material	*pHitsMat	=MakeHitsMat(pTS, pSK);
	Material	*pCharMat	=MakeCharacterMat(pTS, pSK);
	Material	*pTerMat	=MakeTerrainMat(pTS, pSK);
	Material	*pBoxesMat	=MakeNodeBoxesMat(pTS, pSK);
	Material	*pSkyBoxMat	=MakeSkyBoxMat(pTS, pSK);

	//character
	Mesh		*pMesh	=Mesh_Read(pTS->mpGD, pSK, "Characters/Body.mesh");
	Character	*pChar	=Character_Read("Characters/DocuBlender.Character");
	AnimLib		*pALib	=AnimLib_Read("Characters/DocuBlender.AnimLib");

	mat4	bones[MAX_BONES];

	float	animTime		=0.0f;
	float	maxDT			=0.0f;

	pTS->mbRunning	=true;
	while(pTS->mbRunning)
	{
		pTS->mDeltaYaw		=0.0f;
		pTS->mDeltaPitch	=0.0f;

		UpdateTimer_Stamp(pUT);
		while(UpdateTimer_GetUpdateDeltaSeconds(pUT) > 0.0f)
		{
			//zero out charmove
			glm_vec3_zero(pTS->mCharMoveVec);

			//do input here
			//move turn etc
			INP_Update(pInp, pTS);

			if(pTS->mDrawHit == TER_INSIDE)
			{
				MAT_SetSolidColour(pCubeMat, ZAxisCol);
			}
			else if(pTS->mDrawHit != TER_MISS)
			{
				glm_translate_make(hitSphereMat, pTS->mHitPos);

				MAT_SetWorld(pCubeMat, hitSphereMat);

				if(pTS->mDrawHit == TER_HIT)
				{
					MAT_SetSolidColour(pCubeMat, XAxisCol);
				}
				if(pTS->mDrawHit == TER_HIT_INSIDE)
				{
					MAT_SetSolidColour(pCubeMat, YAxisCol);
				}
			}
			else
			{
				MAT_SetSolidColour(pCubeMat, GLM_VEC4_ONE);
			}

			UpdateTimer_UpdateDone(pUT);
		}

		//update materials incase light changed
		MAT_SetLightDirection(pSphereMat, pTS->mLightDir);
		MAT_SetLightDirection(pCubeMat, pTS->mLightDir);
		MAT_SetLightDirection(pRaysMat, pTS->mLightDir);
		MAT_SetLightDirection(pHitsMat, pTS->mLightDir);
		MAT_SetLightDirection(pCharMat, pTS->mLightDir);
		MAT_SetLightDirection(pTerMat, pTS->mLightDir);
		MAT_SetLightDirection(pBoxesMat, pTS->mLightDir);

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		//player moving?
		float	moving	=glm_vec3_norm(pTS->mCharMoveVec);

		if(moving > 0.0f)
		{
			animTime	+=dt * moving * 100.0f;
			AnimLib_Animate(pALib, "DocuWalkBlenderCoords", animTime);
		}
		else
		{
			animTime	+=dt;
			AnimLib_Animate(pALib, "DocuIdleBlenderCoords", animTime);
		}

		Character_FillBoneArray(pChar, AnimLib_GetSkeleton(pALib), bones);

		{
			if(dt > maxDT)
			{
				maxDT	=dt;
			}

			char	timeStr[32];

//			sprintf(timeStr, "maxDT: %f", maxDT);
			sprintf(timeStr, "animTime: %f", animTime);

			ST_ModifyStringText(pTS->mpST, 69, timeStr);
		}

		//update strings
		ST_Update(pTS->mpST, pTS->mpGD);

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
			mat4	viewMat;
			if(pTS->mbFlyMode)
			{
				GameCam_GetViewMatrixFly(pTS->mpCam, viewMat, pTS->mEyePos);
			}
			else
			{
				GameCam_GetViewMatrixThird(pTS->mpCam, viewMat, pTS->mEyePos);

				mat4	headMat, guraMat;
				GameCam_GetLookMatrix(pTS->mpCam, headMat);
				GameCam_GetFlatLookMatrix(pTS->mpCam, guraMat);

				//drop mesh to ground
				vec3	feetToCenter	={	0.0f, sTestBoxMin[1], 0.0f	};
				glm_translate(guraMat, feetToCenter);

				MAT_SetWorld(pCharMat, guraMat);
			}

			CBK_SetView(pCBK, viewMat, pTS->mEyePos);

			//set the skybox world mat to match eye pos
			glm_translate_make(viewMat, pTS->mEyePos);
			MAT_SetWorld(pSkyBoxMat, viewMat);
		}


		PP_SetTargets(pPP, pTS->mpGD, "LinearColor", "LinearDepth");

		PP_ClearDepth(pPP, pTS->mpGD, "LinearDepth");
		PP_ClearTarget(pPP, pTS->mpGD, "LinearColor");

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

		//draw test tri
		{
			GD_IASetVertexBuffers(pTS->mpGD, pPOTri->mpVB, pPOTri->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pPOTri->mpIB, DXGI_FORMAT_R16_UINT, 0);
			MAT_Apply(pHitsMat, pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pPOTri->mIndexCount, 0, 0);
		}

		//draw xyz axis
		CP_DrawAxis(pAxis, pTS->mLightDir, XAxisCol, YAxisCol, ZAxisCol, pCBK, pTS->mpGD);

		//draw test cube
//		GD_IASetVertexBuffers(pTS->mpGD, pCube->mpVB, pCube->mVertSize, 0);
//		GD_IASetIndexBuffers(pTS->mpGD, pCube->mpIB, DXGI_FORMAT_R16_UINT, 0);

//		MAT_Apply(pCubeMat, pCBK, pTS->mpGD);
//		GD_DrawIndexed(pTS->mpGD, pCube->mIndexCount, 0, 0);

		//impact sphere VB/IB etc
		if(pTS->mDrawHit != MISS)
		{
			MAT_SetWorld(pSphereMat, hitSphereMat);

			GD_IASetVertexBuffers(pTS->mpGD, pSphere->mpVB, pSphere->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pSphere->mpIB, DXGI_FORMAT_R16_UINT, 0);

			MAT_Apply(pSphereMat, pCBK, pTS->mpGD);

			GD_DrawIndexed(pTS->mpGD, pSphere->mIndexCount, 0, 0);
		}

		//debug draw quadtree leaf cubes
		if(pTS->mbDrawTerNodes)
		{
			GD_IASetVertexBuffers(pTS->mpGD, pQTBoxes->mpVB, pQTBoxes->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pQTBoxes->mpIB, DXGI_FORMAT_R32_UINT, 0);
			MAT_Apply(pBoxesMat, pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pQTBoxes->mIndexCount, 0, 0);
		}

		//debug draw many rays
		if(pTS->mbDrawManyRays && pTS->mpManyRays != NULL)
		{
			GD_IASetVertexBuffers(pTS->mpGD, pTS->mpManyRays->mpVB, pTS->mpManyRays->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyRays->mpIB, DXGI_FORMAT_R32_UINT, 0);
			MAT_Apply(pRaysMat, pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pTS->mpManyRays->mIndexCount, 0, 0);
		}

		if(pTS->mbDrawManyImpacts && sNumRayImpacts > 0)
		{
			GD_IASetVertexBuffers(pTS->mpGD, pTS->mpManyImpacts->mpVB, pTS->mpManyImpacts->mVertSize, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyImpacts->mpIB, DXGI_FORMAT_R32_UINT, 0);
			MAT_Apply(pHitsMat, pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pTS->mpManyImpacts->mIndexCount, 0, 0);
		}

		//terrain draw
		Terrain_DrawMat(pTS->mpTer, pTS->mpGD, pCBK, pTerMat);

		//set mesh draw stuff
		if(pTS->mbFlyMode)
		{
			glm_translate_make(charMat, pTS->mPlayerPos);
			MAT_SetWorld(pCharMat, charMat);
		}
		MAT_SetDanglyForce(pCharMat, pTS->mDanglyForce);
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

		//bones
		CBK_SetBonesWithTranspose(pCBK, bones);
		CBK_UpdateCharacter(pCBK, pTS->mpGD);
		CBK_SetCharacterToShaders(pCBK, pTS->mpGD);

		//draw mesh
		Mesh_DrawMat(pMesh, pTS->mpGD, pCBK, pCharMat);

		//set proj for 2D
		CBK_SetProjection(pCBK, textProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		ST_Draw(pTS->mpST, pTS->mpGD, pCBK);

		//change back to 3D
		CBK_SetProjection(pCBK, camProj);
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		PP_ClearDepth(pPP, pTS->mpGD, "BackDepth");
		PP_SetTargets(pPP, pTS->mpGD, "BackColor", "BackDepth");

		PP_SetSRV(pPP, pTS->mpGD, "LinearColor", 1);	//1 for colortex

		PP_DrawStage(pPP, pTS->mpGD, pCBK);

		GD_Present(pTS->mpGD);
	}

	GD_Destroy(&pTS->mpGD);

	return	EXIT_SUCCESS;
}


static void	ReBuildManyRayPrims(PrimObject **ppMR, PrimObject **ppMI, GraphicsDevice *pGD)
{
	//free previous
	if(*ppMR != NULL)
	{
		PF_DestroyPO(ppMR);
	}
	if(*ppMI != NULL)
	{
		PF_DestroyPO(ppMI);
	}

	*ppMR	=PF_CreateManyRays(sRayStarts, sRayEnds, sRayResults, NUM_RAYS, RAY_WIDTH, pGD);

	*ppMI	=PF_CreateManyCubes(sRayImpacts, sNumRayImpacts, IMPACT_WIDTH, pGD);
}

//input handlers
static void	PrintRandomPointInTerrain(const Terrain *pTer)
{
	vec3	randPoint, terMins, terMaxs;

	Terrain_GetBounds(pTer, terMins, terMaxs);

	Misc_RandomPointInBound(terMins, terMaxs, randPoint);

	printf("Random Point: %.2f, %.2f, %.2f\n", randPoint[0], randPoint[1], randPoint[2]);
}

//throw a large amount of rays at the terrain
static void	TestManyRays(const Terrain *pTer)
{
	vec3	terMins, terMaxs;

	sNumRayImpacts	=0;

	Terrain_GetBounds(pTer, terMins, terMaxs);

	//bump mins down to make rays more likely to hit the ground
	terMins[1]	-=50.0f;

	//ray colours for hit and miss
	//eventually will have start inside as well
	vec4	hitCol	={	1.0f,	0.0f,	0.0f,	1.0f	};
	vec4	missCol	={	0.0f,	1.0f,	0.0f,	1.0f	};

	__uint128_t	startTime	=__rdtsc();

	for(int i=0;i < NUM_RAYS;i++)
	{
		vec3	start, end;
		vec3	hit;
		vec4	hitPlane;

		Misc_RandomPointInBound(terMins, terMaxs, start);
		Misc_RandomPointInBound(terMins, terMaxs, end);

		//store
		glm_vec3_copy(start, sRayStarts[i]);
		glm_vec3_copy(end, sRayEnds[i]);

		//invalidate hit point
		glm_vec3_fill(hit, FLT_MAX);
/*
		if(Terrain_LineIntersect(pTer, start, end, hit, hitPlane))
		{
			glm_vec4_copy(hitCol, sRayResults[i]);

			glm_vec3_copy(hit, sRayImpacts[sNumRayImpacts]);
			sNumRayImpacts++;
		}
		else
		{
			glm_vec4_copy(missCol, sRayResults[i]);
		}*/
	}

	__uint128_t	endTime	=__rdtsc();

	uint64_t	delta	=(endTime - startTime);

	int	numRays	=NUM_RAYS;

	printf("Ray test of %d rays took %lu tics\n", numRays, delta);
}

static int	TestOneRay(const Terrain *pTer, const GameCamera *pCam, const vec3 eyePos, vec3 hitPos, vec4 hitPlane)
{
	vec3	forward, right, up, endRay;

	GameCam_GetForwardVec(pCam, forward);
	GameCam_GetRightVec(pCam, right);
	GameCam_GetUpVec(pCam, up);

	glm_vec3_scale(forward, RAY_LEN, endRay);
	glm_vec3_add(eyePos, endRay, endRay);

	//invalidate hit point
	glm_vec3_fill(hitPos, FLT_MAX);

	//convert to a ray format
	vec3	rayDir;

	glm_vec3_sub(endRay, eyePos, rayDir);
	glm_vec3_scale(rayDir, 1.0f / RAY_LEN, rayDir);

	vec3	invDir;
	Misc_SSE_ReciprocalVec3(rayDir, invDir);

	vec3	bounds[2];
	glm_vec3_copy(sTestBoxMin, bounds[0]);
	glm_vec3_copy(sTestBoxMax, bounds[1]);

	Misc_ExpandBounds(bounds[0], bounds[1], 0.5f);

//	bool	bHit	=Terrain_LineIntersect(pTer, eyePos, endRay, hitPos, hitPlane);
//	bool	bHit	=Terrain_CapsuleIntersect(pTer, eyePos, endRay, 0.5f, hitPos, hitPlane);
//	bool	bHit	=Misc_CapsuleIntersectBounds(sTestBoxMin, sTestBoxMax, eyePos, endRay, 0.5f, hitPos, hitPlane);
//	int	res	=CV_SweptSphereIntersect(spTestVol, eyePos, endRay, 0.5f, hitPos, hitPlane);
//	int	res	=Terrain_SweptBoundIntersect(pTer, eyePos, endRay, sTestBoxMin, sTestBoxMax, hitPos, hitPlane);
//	int	res	=PM_SweptSphereToTriIntersect(sTestTri, eyePos, endRay, 0.25f, hitPos, hitPlane);
	int	res	=Terrain_SweptSphereIntersect(pTer, eyePos, endRay, 0.25f, hitPos, hitPlane);

	return	res;
}

static int	TestOneMove(const Terrain *pTer, vec3 hitPos, vec4 hitPlane)
{
	//test a simple move vector with a bound
	vec3	start	={	45.665318f, 3.499443f, 54.451221f	};	//move straight x
	vec3	end		={	49.756111f, 3.303342f, 50.492950f	};	//3 units and down

	//invalidate hit point
	glm_vec3_fill(hitPos, FLT_MAX);

//	bool	bGood	=Terrain_MoveBox(pTer, sTestBoxMin, sTestBoxMax, start, end, hitPos);

	return	VOL_HIT_VISIBLE;
}

//event handlers (eh)
static void	RandLightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	Misc_RandomDirection(pTS->mLightDir);
}

static void	DangleDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDanglyForce[0]	-=UVSCALE_RATE;
}

static void	DangleUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDanglyForce[0]	+=UVSCALE_RATE;
}

static void	CastOneRayEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDrawHit	=TestOneRay(pTS->mpTer, pTS->mpCam, pTS->mEyePos, pTS->mHitPos, pTS->mHitPlane);
//	pTS->mDrawHit	=TestOneMove(pTS->mpTer, pTS->mHitPos, pTS->mHitPlane);
}

static void	PrintRandomEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	PrintRandomPointInTerrain(pTS->mpTer);
}

static void	CastManyRaysEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	TestManyRays(pTS->mpTer);
	ReBuildManyRayPrims(&pTS->mpManyRays, &pTS->mpManyImpacts, pTS->mpGD);
}

static void	ToggleDrawTerNodesEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbDrawTerNodes	=!pTS->mbDrawTerNodes;
}

static void	ToggleDrawManyRaysEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbDrawManyRays	=!pTS->mbDrawManyRays;
}

static void	ToggleDrawManyImpactsEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbDrawManyImpacts	=!pTS->mbDrawManyImpacts;
}

static void	ToggleFlyModeEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbFlyMode	=!pTS->mbFlyMode;
}

static void	LeftMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	LeftMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);
}

static void	RightMouseDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	pTS->mbMouseLooking	=true;
}

static void	RightMouseUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	SDL_SetRelativeMouseMode(SDL_FALSE);

	pTS->mbMouseLooking	=false;
}

static void	MouseMoveEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	if(pTS->mbMouseLooking)
	{
		pTS->mDeltaYaw		+=(pEvt->motion.xrel * MOUSE_TO_ANG);
		pTS->mDeltaPitch	+=(pEvt->motion.yrel * MOUSE_TO_ANG);
	}
}

static void	KeyMoveForwardEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	//accumulate movement
	glm_vec3_add(forward, pTS->mCharMoveVec, pTS->mCharMoveVec);

	if(pTS->mbFlyMode)
	{
		glm_vec3_add(pTS->mEyePos, forward, pTS->mEyePos);
	}
	else
	{
		vec3	end, newPos;
		glm_vec3_add(pTS->mPlayerPos, forward, end);
/*		if(Terrain_MoveBox(pTS->mpTer, sTestBoxMin, sTestBoxMax, pTS->mPlayerPos, end, newPos))
		{
			//watch for a glitchy move
			float	dist	=glm_vec3_distance(pTS->mPlayerPos, newPos);
			if(dist > 10.0f)
			{
				printf("Glitchy Move: %f %f %f to %f %f %f\n",
					pTS->mPlayerPos[0], pTS->mPlayerPos[1], pTS->mPlayerPos[2], 
					end[0], end[1], end[2]);
			}

			glm_vec3_copy(newPos, pTS->mPlayerPos);

			ST_ModifyStringText(pTS->mpST, 70, "Moved!");
		}
		else*/
		{
			ST_ModifyStringText(pTS->mpST, 70, "Move error!");
		}
	}
}

static void	KeyMoveBackEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, -MOVE_RATE, forward);

	//accumulate movement
	glm_vec3_add(forward, pTS->mCharMoveVec, pTS->mCharMoveVec);

	glm_vec3_add(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	KeyMoveLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, -MOVE_RATE, right);

	//accumulate movement
	glm_vec3_add(right, pTS->mCharMoveVec, pTS->mCharMoveVec);

	glm_vec3_add(pTS->mEyePos, right, pTS->mEyePos);
}

static void	KeyMoveRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

	//accumulate movement
	glm_vec3_add(right, pTS->mCharMoveVec, pTS->mCharMoveVec);

	glm_vec3_add(pTS->mEyePos, right, pTS->mEyePos);
}

static void	KeyMoveUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	up;
	GameCam_GetUpVec(pTS->mpCam, up);
	glm_vec3_scale(up, MOVE_RATE, up);

	glm_vec3_add(pTS->mEyePos, up, pTS->mEyePos);
}

static void	KeyMoveDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	up;
	GameCam_GetUpVec(pTS->mpCam, up);
	glm_vec3_scale(up, MOVE_RATE, up);

	glm_vec3_sub(pTS->mEyePos, up, pTS->mEyePos);
}

static void	KeyTurnLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	-=KEYTURN_RATE;
}

static void	KeyTurnRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	+=KEYTURN_RATE;
}

static void	KeyTurnUpEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaPitch	+=KEYTURN_RATE;
}

static void	KeyTurnDownEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaPitch	-=KEYTURN_RATE;
}

static void	EscEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mbRunning	=false;
}

static Material	*MakeSphereMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};

	MAT_SetLayout(pRet, "VPosNorm", pSK);
	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVS", pSK);
	MAT_SetPShader(pRet, "TriSolidSpecPS", pSK);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 6.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeCubeMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};

	MAT_SetLayout(pRet, "VPosNorm", pSK);
	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVS", pSK);
	MAT_SetPShader(pRet, "TriSolidSpecPS", pSK);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 6.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeRaysMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.5f, 0.5f, 0.5f	};
	vec3	light2		={	0.3f, 0.3f, 0.3f	};

	MAT_SetLayout(pRet, "VPosNormCol0", pSK);
	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVColorVS", pSK);
	MAT_SetPShader(pRet, "TriSolidVColorSpecPS", pSK);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 4.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeHitsMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f		};
	vec3	light1		={	0.5f, 0.5f, 0.5f		};
	vec3	light2		={	0.3f, 0.3f, 0.3f		};
	vec4	hitCol		={	1.0f, 0.0f, 0.0f, 1.0f	};

	MAT_SetLayout(pRet, "VPosNorm", pSK);
	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVS", pSK);
	MAT_SetPShader(pRet, "TriSolidSpecPS", pSK);
	MAT_SetSolidColour(pRet, hitCol);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 4.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeCharacterMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};

	MAT_SetLayout(pRet, "VPosNormBoneTex0", pSK);
	MAT_SetVShader(pRet, "SkinWNormWPosTex0VS", pSK);
	MAT_SetPShader(pRet, "TriTex0SpecPS", pSK);
	MAT_SetSRV0(pRet, "Characters/Docu", pSK);

	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 6.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeTerrainMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};

	MAT_SetLayout(pRet, "VPosNormTex04Tex14", pSK);
	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosTexFactVS", pSK);
	MAT_SetPShader(pRet, "TriTexFact8PS", pSK);
	MAT_SetSolidColour(pRet, GLM_VEC4_ONE);
	MAT_SetSRV0(pRet, "Terrain/TerAtlas", pSK);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 3.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeNodeBoxesMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	vec3	light0		={	1.0f, 1.0f, 1.0f		};
	vec3	light1		={	0.5f, 0.5f, 0.5f		};
	vec3	light2		={	0.2f, 0.2f, 0.2f		};
	vec4	ghosty		={	1.0f, 1.0f, 1.0f, 0.5f	};

	MAT_SetLayout(pRet, "VPosNorm", pSK);
	MAT_SetLights(pRet, light0, light1, light2, pTS->mLightDir);
	MAT_SetVShader(pRet, "WNormWPosVS", pSK);
	MAT_SetPShader(pRet, "TriSolidSpecPS", pSK);
	MAT_SetSolidColour(pRet, ghosty);
	MAT_SetSpecular(pRet, GLM_VEC3_ONE, 6.0f);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static Material	*MakeSkyBoxMat(TestStuff *pTS, const StuffKeeper *pSK)
{
	Material	*pRet	=MAT_Create(pTS->mpGD);

	MAT_SetLayout(pRet, "VPosNormTex0", pSK);
	MAT_SetVShader(pRet, "SkyBoxVS", pSK);
	MAT_SetPShader(pRet, "SkyGradientFogPS", pSK);
	MAT_SetWorld(pRet, GLM_MAT4_IDENTITY);

	return	pRet;
}

static void	SetupKeyBinds(Input *pInp)
{
	//event style bindings
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_l, RandLightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_i, CastOneRayEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_p, PrintRandomEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_u, CastManyRaysEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_n, ToggleDrawTerNodesEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_m, ToggleDrawManyRaysEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_b, ToggleDrawManyImpactsEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_f, ToggleFlyModeEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, EscEH);

	//held bindings
	//movement
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_w, KeyMoveForwardEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_a, KeyMoveLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_s, KeyMoveBackEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_d, KeyMoveRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_c, KeyMoveUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_z, KeyMoveDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_LEFT, DangleDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_RIGHT, DangleUpEH);

	//key turning
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_q, KeyTurnLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_e, KeyTurnRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_r, KeyTurnUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_t, KeyTurnDownEH);

	//move data events
	INP_MakeBinding(pInp, INP_BIND_TYPE_MOVE, SDL_MOUSEMOTION, MouseMoveEH);

	//down/up events
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_RIGHT, RightMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_RIGHT, RightMouseUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_PRESS, SDL_BUTTON_LEFT, LeftMouseDownEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_RELEASE, SDL_BUTTON_LEFT, LeftMouseUpEH);
}

static void	SetupRastVP(GraphicsDevice *pGD)
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

static void SetupDebugStrings(TestStuff *pTS, const StuffKeeper *pSK)
{
	//debug screen text
	pTS->mpST	=ST_Create(pTS->mpGD, pSK, MAX_ST_CHARS, "CGA", "CGA");

	vec2	embiggen	={	2.0f, 2.0f	};
	vec2	topLeftPos	={	5.0f, 5.0f	};
	vec2	nextLine	={	0.0f, 20.0f	};
	vec4	red			={	1.0f, 0.0f, 0.0f, 1.0f	};
	__attribute_maybe_unused__
	vec4	blue		={	0.0f, 0.0f, 1.0f, 1.0f	};
	vec4	green		={	0.0f, 1.0f, 0.0f, 1.0f	};
	vec4	magenta		={	1.0f, 0.0f, 1.0f, 1.0f	};
	__attribute_maybe_unused__
	vec4	cyan		={	0.0f, 1.0f, 1.0f, 1.0f	};


	ST_AddString(pTS->mpST, "Timing thing", 69, green, topLeftPos, embiggen);

	glm_vec2_add(topLeftPos, nextLine, topLeftPos);
	ST_AddString(pTS->mpST, "Movestuff", 70, red, topLeftPos, embiggen);

	glm_vec2_add(topLeftPos, nextLine, topLeftPos);
	ST_AddString(pTS->mpST, "Pos Storage", 71, magenta, topLeftPos, embiggen);
}

static void	SetThirdPersonCam(TestStuff *pTS, GameCamera *pGC)
{
//	GCam
}