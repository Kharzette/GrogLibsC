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
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/MiscStuff.h"
#include	"UtilityLib/ConvexVolume.h"
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

//should match CommonFunctions.hlsli
#define	MAX_BONES		55

//data to store test rays
static vec3	sRayStarts[NUM_RAYS];
static vec3	sRayEnds[NUM_RAYS];
static vec4	sRayResults[NUM_RAYS];
static vec3	sRayImpacts[NUM_RAYS];
static int	sNumRayImpacts;

//input context, stuff input handlers will need
typedef struct	TestStuff_t
{
	GraphicsDevice	*mpGD;
	Terrain			*mpTer;
	GameCamera		*mpCam;
	PrimObject		*mpManyRays;
	PrimObject		*mpManyImpacts;

	//toggles
	bool	mbDrawTerNodes;
	bool	mbDrawManyRays;
	bool	mbDrawManyImpacts;
	bool	mbMouseLooking;
	bool	mbRunning;

	//misc data
	vec3	mDanglyForce;
	vec3	mLightDir;
	vec3	mEyePos;
	float	mDeltaYaw, mDeltaPitch;

	//single ray cast vars
	bool	mbDrawHit;
	vec3	mHitPos;
	vec4	mHitPlane;
}	TestStuff;

//static forward decs
static void	TestManyRays(const Terrain *pTer);
static void	PrintRandomPointInTerrain(const Terrain *pTer);
static bool	TestOneRay(const Terrain *pTer, const GameCamera *pCam, const vec3 eyePos, vec3 hitPos, vec4 hitPlane);
static void	KeyTurnHandler(const SDL_Event *pEvt, float *pDeltaYaw, float *pDeltaPitch);
static bool	KeyMovementHandler(GameCamera *pCam, vec3 eyePos, const SDL_Event *pEvt);
static void	ReBuildManyRayPrims(PrimObject **ppMR, PrimObject **ppMI, GraphicsDevice *pGD);

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

	Input	*pInp	=INP_CreateInput();

	//event style bindings
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_l, RandLightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_i, CastOneRayEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_p, PrintRandomEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_u, CastManyRaysEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_n, ToggleDrawTerNodesEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_m, ToggleDrawManyRaysEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_b, ToggleDrawManyImpactsEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_EVENT, SDLK_ESCAPE, EscEH);

	//held bindings
	//movement
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_w, KeyMoveForwardEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_a, KeyMoveLeftEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_s, KeyMoveBackEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_d, KeyMoveRightEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_c, KeyMoveUpEH);
	INP_MakeBinding(pInp, INP_BIND_TYPE_HELD, SDLK_z, KeyMoveDownEH);

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


	GD_Init(&pTS->mpGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_0);

	StuffKeeper	*pSK	=StuffKeeper_Create(pTS->mpGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pTS->mpGD);
		return	EXIT_FAILURE;
	}

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
	ID3D11RasterizerState	*pRast	=GD_CreateRasterizerState(pTS->mpGD, &rastDesc);

	pTS->mpTer	=Terrain_Create(pTS->mpGD, "Blort", "Textures/Terrain/HeightMaps/MZCloud.png", 10, HEIGHT_SCALAR);

	//debugdraw quadtree boxes
	int		numBounds;
	vec3	*pMins, *pMaxs;
	Terrain_GetQuadTreeLeafBoxes(pTS->mpTer, &pMins, &pMaxs, &numBounds);

	PrimObject	*pQTBoxes	=PF_CreateCubesFromBoundArray(pMins, pMaxs, numBounds, pTS->mpGD);

	//test prims
	PrimObject	*pCube	=PF_CreateSphere(GLM_VEC3_ZERO, 0.5f, pTS->mpGD);
	LightRay	*pLR	=CP_CreateLightRay(5.0f, 0.25f, pTS->mpGD);
	AxisXYZ		*pAxis	=CP_CreateAxis(5.0f, 0.1f, pTS->mpGD);

	CBKeeper	*pCBK	=CBK_Create(pTS->mpGD);

	PostProcess	*pPP	=PP_Create(pTS->mpGD, pSK, pCBK);

	PP_MakePostTarget(pPP, pTS->mpGD, "LinearColor", RESX, RESY, DXGI_FORMAT_R8G8B8A8_UNORM);
	PP_MakePostDepth(pPP, pTS->mpGD, "LinearDepth", RESX, RESY, DXGI_FORMAT_D32_FLOAT);

	float	aspect	=(float)RESX / (float)RESY;

	mat4	charMat, hitSphereMat;

	pTS->mEyePos[1]	=0.6f;
	pTS->mEyePos[2]	=4.5f;

	pTS->mpCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 30.0f);

	//projection won't change in this test program, probably
	{
		mat4	proj;
		GameCam_GetProjection(pTS->mpCam, proj);
		CBK_SetProjection(pCBK, proj);
	}

	//good old xna blue
	float	clearColor[]	={ 100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f };

	D3D11_VIEWPORT	vp;

	vp.Width	=RESX;
	vp.Height	=RESY;
	vp.MaxDepth	=1.0f;
	vp.MinDepth	=0.0f;
	vp.TopLeftX	=0;
	vp.TopLeftY	=0;

	GD_RSSetViewPort(pTS->mpGD, &vp);

	//set constant buffers to shaders, think I just have to do this once
	GD_VSSetShader(pTS->mpGD, StuffKeeper_GetVertexShader(pSK, "SkinWNormWPosTex0VS"));
	GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));	
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	//for each set of shaders used?
	GD_VSSetShader(pTS->mpGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexFactVS"));
	GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriTexFact8PS"));	
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	GD_VSSetShader(pTS->mpGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
	GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));	
	CBK_SetCommonCBToShaders(pCBK, pTS->mpGD);

	GD_RSSetState(pTS->mpGD, pRast);
	GD_IASetInputLayout(pTS->mpGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
	GD_IASetPrimitiveTopology(pTS->mpGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	vec4	specColor	={	1.0f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor0	={	1.0f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor1	={	0.5f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor2	={	1.0f, 0.5f, 1.0f, 1.0f	};
	vec4	lightRayCol	={	1.0f, 1.0f, 0.0f, 1.0f	};
	vec4	XAxisCol	={	1.0f, 0.0f, 0.0f, 1.0f	};
	vec4	YAxisCol	={	0.0f, 0.0f, 1.0f, 1.0f	};
	vec4	ZAxisCol	={	0.0f, 1.0f, 0.0f, 1.0f	};
	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};
	vec3	skyGrad0	={	0.7f, 0.2f, 0.1f	};
	vec3	skyGrad1	={	0.2f, 0.3f, 1.0f	};

	pTS->mLightDir[0]		=0.3f;
	pTS->mLightDir[1]		=-0.7f;
	pTS->mLightDir[2]		=-0.5f;
	pTS->mDanglyForce[0]	=700.0f;

	glm_vec3_normalize(pTS->mLightDir);
	glm_mat4_identity(charMat);

	CBK_SetFogVars(pCBK, 500.0f, 1000.0f, true);
	CBK_SetSky(pCBK, skyGrad0, skyGrad1);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);

	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

	//character
	Mesh		*pMesh	=Mesh_Read(pTS->mpGD, pSK, "Characters/Body.mesh");
	Character	*pChar	=Character_Read("Characters/DocuBlender.Character");
	AnimLib		*pALib	=AnimLib_Read("Characters/DocuBlender.AnimLib");

	mat4	bones[MAX_BONES];

	float	animTime	=0.0f;

	pTS->mbRunning	=true;
	while(pTS->mbRunning)
	{
		pTS->mDeltaYaw		=0.0f;
		pTS->mDeltaPitch	=0.0f;

		UpdateTimer_Stamp(pUT);
		while(UpdateTimer_GetUpdateDeltaSeconds(pUT) > 0.0f)
		{
//			vec3	forward, right, up;

//			GameCam_GetForwardVec(pCam, forward);
//			GameCam_GetRightVec(pCam, right);
//			GameCam_GetUpVec(pCam, up);

			INP_Update(pInp, pTS);
/*
			SDL_Event	evt;
			while(SDL_PollEvent(&evt))
			{
				bRunning	=KeyMovementHandler(pCam, eyePos, &evt);

				KeyTurnHandler(&evt, &deltaYaw, &deltaPitch);

				if(evt.type == SDL_KEYDOWN)
				{
					if(evt.key.keysym.sym == SDLK_LEFT)
					{
						dangly[0]	-=UVSCALE_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_RIGHT)
					{
						dangly[0]	+=UVSCALE_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_l)
					{
						Misc_RandomDirection(lightDir);
					}
					else if(evt.key.keysym.sym == SDLK_i)
					{
						bDrawHit	=TestOneRay(pTer, pCam, eyePos, hitPos, hitPlane);

						if(bDrawHit)
						{
							glm_translate_make(hitSphereMat, hitPos);
						}
					}
					else if(evt.key.keysym.sym == SDLK_p)
					{
						PrintRandomPointInTerrain(pTer);
					}
					else if(evt.key.keysym.sym == SDLK_u)
					{
						TestManyRays(pTer);
						ReBuildManyRayPrims(&pManyRays, &pManyImpacts, pGD);
					}
					else if(evt.key.keysym.sym == SDLK_n)
					{
						bDrawTerNodes	=!bDrawTerNodes;
					}
					else if(evt.key.keysym.sym == SDLK_m)
					{
						bDrawManyRays	=!bDrawManyRays;
					}
					else if(evt.key.keysym.sym == SDLK_b)
					{
						bDrawManyImpacts	=!bDrawManyImpacts;
					}
				}
				else if(evt.type == SDL_MOUSEBUTTONDOWN)
				{
					if(evt.button.button == SDL_BUTTON_RIGHT)
					{
						int	capResult	=SDL_SetRelativeMouseMode(SDL_TRUE);
						printf("Right button down: %d\n", capResult);
						bMouseLooking	=(capResult == 0);
					}
					else if(evt.button.button == SDL_BUTTON_LEFT)
					{
						printf("Left button down\n");
					}
				}
				else if(evt.type == SDL_MOUSEBUTTONUP)
				{
					if(evt.button.button == SDL_BUTTON_RIGHT)
					{
						int	capResult	=SDL_SetRelativeMouseMode(SDL_FALSE);
						printf("Right button up: %d\n", capResult);
						bMouseLooking	=false;
					}
					else if(evt.button.button == SDL_BUTTON_LEFT)
					{
						printf("Left button up\n");
					}
				}
				else if(evt.type == SDL_MOUSEMOTION)
				{
					if(bMouseLooking)
					{
						deltaYaw	-=(evt.motion.xrel * MOUSE_TO_ANG);
						deltaPitch	-=(evt.motion.yrel * MOUSE_TO_ANG);
					}
				}
			}
			*/

			//do input here
			//move turn etc
			if(pTS->mbDrawHit)
			{
				glm_translate_make(hitSphereMat, pTS->mHitPos);
			}

			UpdateTimer_UpdateDone(pUT);
		}

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		animTime	+=dt;
//		AnimLib_Animate(pALib, "DocuBaseBlenderCoords", animTime);
//		AnimLib_Animate(pALib, "DocuWalkBlenderCoords", animTime);
		AnimLib_Animate(pALib, "DocuIdleBlenderCoords", animTime);
		
		Character_FillBoneArray(pChar, AnimLib_GetSkeleton(pALib), bones);

		//set no blend, I think post processing turns it on maybe
		GD_OMSetBlendState(pTS->mpGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

		//camera update
		GameCam_Update(pTS->mpCam, pTS->mEyePos, pTS->mDeltaPitch, pTS->mDeltaYaw, 0.0f);

		//set CB view
		{
			vec3	zeroVec		={	0.0f, 0.0f, 0.0f		};
			vec4	idntQuat	={	0.0f, 0.0f, 0.0f, 1.0f	};
			mat4	viewMat;
			vec4	viewQuat;
			vec3	negPos;

			GameCam_GetViewMatrixFly(pTS->mpCam, viewMat, pTS->mEyePos, viewQuat);

			glm_vec3_negate_to(pTS->mEyePos, negPos);

			CBK_SetView(pCBK, viewMat, negPos);
		}


		PP_SetTargets(pPP, pTS->mpGD, "LinearColor", "LinearDepth");

		GD_OMSetDepthStencilState(pTS->mpGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		PP_ClearDepth(pPP, pTS->mpGD, "LinearDepth");
		PP_ClearTarget(pPP, pTS->mpGD, "LinearColor");

		//set input layout for the prim draws
		GD_IASetInputLayout(pTS->mpGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
		GD_VSSetShader(pTS->mpGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
		GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriSolidSpecPS"));

		//update frame CB
		CBK_UpdateFrame(pCBK, pTS->mpGD);

		//draw light ray
		GD_PSSetSRV(pTS->mpGD, NULL, 0);
		CP_DrawLightRay(pLR, pTS->mLightDir, lightRayCol, pCBK, pTS->mpGD);

		//draw xyz axis
		CP_DrawAxis(pAxis, pTS->mLightDir, XAxisCol, YAxisCol, ZAxisCol, pCBK, pTS->mpGD);

		//impact sphere VB/IB etc
		if(pTS->mbDrawHit)
		{
			GD_IASetVertexBuffers(pTS->mpGD, pCube->mpVB, 24, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pCube->mpIB, DXGI_FORMAT_R16_UINT, 0);
			GD_PSSetSRV(pTS->mpGD, StuffKeeper_GetSRV(pSK, "Floors/Floor13"), 0);
			GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));
			CBK_SetSpecular(pCBK, specColor, 6.0f);
			CBK_SetTrilights3(pCBK, light0, light1, light2, pTS->mLightDir);
			CBK_SetSolidColour(pCBK, solidColor0);

			CBK_SetWorldMat(pCBK, hitSphereMat);
			CBK_SetSolidColour(pCBK, solidColor0);
			CBK_UpdateObject(pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pCube->mIndexCount, 0, 0);
		}

		//debug draw quadtree leaf cubes
		if(pTS->mbDrawTerNodes)
		{
			GD_IASetVertexBuffers(pTS->mpGD, pQTBoxes->mpVB, 24, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pQTBoxes->mpIB, DXGI_FORMAT_R32_UINT, 0);
			CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
			CBK_UpdateObject(pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pQTBoxes->mIndexCount, 0, 0);
		}

		//debug draw many rays
		if(pTS->mbDrawManyRays && pTS->mpManyRays != NULL)
		{
			GD_IASetInputLayout(pTS->mpGD, StuffKeeper_GetInputLayout(pSK, "VPosNormCol0"));
			GD_VSSetShader(pTS->mpGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosVColorVS"));
			GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriSolidVColorSpecPS"));

			GD_IASetVertexBuffers(pTS->mpGD, pTS->mpManyRays->mpVB, 28, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyRays->mpIB, DXGI_FORMAT_R32_UINT, 0);
			CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
			CBK_UpdateObject(pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pTS->mpManyRays->mIndexCount, 0, 0);
		}

		if(pTS->mbDrawManyImpacts && sNumRayImpacts > 0)
		{
			GD_IASetInputLayout(pTS->mpGD, StuffKeeper_GetInputLayout(pSK, "VPosNormCol0"));
			GD_VSSetShader(pTS->mpGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosVColorVS"));
			GD_PSSetShader(pTS->mpGD, StuffKeeper_GetPixelShader(pSK, "TriSolidVColorSpecPS"));

			GD_IASetVertexBuffers(pTS->mpGD, pTS->mpManyImpacts->mpVB, 28, 0);
			GD_IASetIndexBuffers(pTS->mpGD, pTS->mpManyImpacts->mpIB, DXGI_FORMAT_R32_UINT, 0);
			CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
			CBK_UpdateObject(pCBK, pTS->mpGD);
			GD_DrawIndexed(pTS->mpGD, pTS->mpManyImpacts->mIndexCount, 0, 0);
		}

		//set up terrain draw
		CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
		CBK_UpdateObject(pCBK, pTS->mpGD);
		Terrain_Draw(pTS->mpTer, pTS->mpGD, pSK);

		//set mesh draw stuff
		CBK_SetWorldMat(pCBK, charMat);
		CBK_SetDanglyForce(pCBK, pTS->mDanglyForce);
		CBK_SetSolidColour(pCBK, solidColor0);
		CBK_UpdateObject(pCBK, pTS->mpGD);
		GD_PSSetSampler(pTS->mpGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

		//bones
		CBK_SetBonesWithTranspose(pCBK, bones);
		CBK_UpdateCharacter(pCBK, pTS->mpGD);
		CBK_SetCharacterToShaders(pCBK, pTS->mpGD);

		//draw mesh
		Mesh_Draw(pMesh, pTS->mpGD, pSK, "SkinWNormWPosTex0VS", "TriTex0SpecPS", "Characters/Docu");

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

	vec4	impactColour	={	1.0f,	0.0f,	0.0f,	1.0f	};

	*ppMR	=PF_CreateManyRays(sRayStarts, sRayEnds, sRayResults, NUM_RAYS, RAY_WIDTH, pGD);

	*ppMI	=PF_CreateManyCubes(sRayImpacts, impactColour, sNumRayImpacts, IMPACT_WIDTH, pGD);
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

		if(Terrain_LineIntersect(pTer, start, end, hit, hitPlane))
		{
			glm_vec4_copy(hitCol, sRayResults[i]);

			glm_vec3_copy(hit, sRayImpacts[sNumRayImpacts]);
			sNumRayImpacts++;
		}
		else
		{
			glm_vec4_copy(missCol, sRayResults[i]);
		}
	}

	__uint128_t	endTime	=__rdtsc();

	uint64_t	delta	=(endTime - startTime);

	int	numRays	=NUM_RAYS;

	printf("Ray test of %d rays took %lu tics\n", numRays, delta);
}

static bool	TestOneRay(const Terrain *pTer, const GameCamera *pCam, const vec3 eyePos, vec3 hitPos, vec4 hitPlane)
{
	vec3	forward, right, up, endRay;

	GameCam_GetForwardVec(pCam, forward);
	GameCam_GetRightVec(pCam, right);
	GameCam_GetUpVec(pCam, up);

	glm_vec3_scale(forward, -RAY_LEN, endRay);
	glm_vec3_add(eyePos, endRay, endRay);

	//invalidate hit point
	glm_vec3_fill(hitPos, FLT_MAX);

	bool	bHit	=Terrain_LineIntersect(pTer, eyePos, endRay, hitPos, hitPlane);

	return	bHit;
}

static bool	KeyMovementHandler(GameCamera *pCam, vec3 eyePos, const SDL_Event *pEvt)
{
	vec3	forward, right, up;

	GameCam_GetForwardVec(pCam, forward);
	GameCam_GetRightVec(pCam, right);
	GameCam_GetUpVec(pCam, up);

	if(pEvt->type == SDL_QUIT)
	{
		return	false;
	}
	else if(pEvt->type == SDL_KEYDOWN)
	{
		if(pEvt->key.keysym.sym == SDLK_w)
		{
			glm_vec3_sub(eyePos, forward, eyePos);
		}
		else if(pEvt->key.keysym.sym == SDLK_a)
		{
			glm_vec3_sub(eyePos, right, eyePos);
		}
		else if(pEvt->key.keysym.sym == SDLK_s)
		{
			glm_vec3_add(eyePos, forward, eyePos);
		}
		else if(pEvt->key.keysym.sym == SDLK_d)
		{
			glm_vec3_add(eyePos, right, eyePos);
		}
		else if(pEvt->key.keysym.sym == SDLK_z)
		{
			glm_vec3_sub(eyePos, up, eyePos);
		}
		else if(pEvt->key.keysym.sym == SDLK_c)
		{
			glm_vec3_add(eyePos, up, eyePos);
		}
		else if(pEvt->key.keysym.sym == SDLK_ESCAPE)
		{
			return	false;
		}
	}
	return	true;
}

static void	KeyTurnHandler(const SDL_Event *pEvt, float *pDeltaYaw, float *pDeltaPitch)
{
	if(pEvt->type == SDL_KEYDOWN)
	{
		if(pEvt->key.keysym.sym == SDLK_q)
		{
			*pDeltaYaw	+=KEYTURN_RATE;
		}
		else if(pEvt->key.keysym.sym == SDLK_e)
		{
			*pDeltaYaw	+=-KEYTURN_RATE;
		}
		else if(pEvt->key.keysym.sym == SDLK_r)
		{
			*pDeltaPitch	+=KEYTURN_RATE;
		}
		else if(pEvt->key.keysym.sym == SDLK_t)
		{
			*pDeltaPitch	+=-KEYTURN_RATE;
		}
	}
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

	pTS->mbDrawHit	=TestOneRay(pTS->mpTer, pTS->mpCam, pTS->mEyePos, pTS->mHitPos, pTS->mHitPlane);
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
		pTS->mDeltaYaw		-=(pEvt->motion.xrel * MOUSE_TO_ANG);
		pTS->mDeltaPitch	-=(pEvt->motion.yrel * MOUSE_TO_ANG);
	}
}

static void	KeyMoveForwardEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	glm_vec3_sub(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	KeyMoveBackEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	forward;
	GameCam_GetForwardVec(pTS->mpCam, forward);
	glm_vec3_scale(forward, MOVE_RATE, forward);

	glm_vec3_add(pTS->mEyePos, forward, pTS->mEyePos);
}

static void	KeyMoveLeftEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

	glm_vec3_sub(pTS->mEyePos, right, pTS->mEyePos);
}

static void	KeyMoveRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	vec3	right;
	GameCam_GetRightVec(pTS->mpCam, right);
	glm_vec3_scale(right, MOVE_RATE, right);

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

	pTS->mDeltaYaw	+=KEYTURN_RATE;
}

static void	KeyTurnRightEH(void *pContext, const SDL_Event *pEvt)
{
	TestStuff	*pTS	=(TestStuff *)pContext;

	assert(pTS);

	pTS->mDeltaYaw	-=KEYTURN_RATE;
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
