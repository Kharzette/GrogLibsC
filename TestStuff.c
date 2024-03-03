#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
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


#define	RESX			800
#define	RESY			600
#define	ROT_RATE		10.0f
#define	UVSCALE_RATE	1.0f
#define	KEYTURN_RATE	0.1f
#define	HEIGHT_SCALAR	0.5f
#define	RAY_LEN			100.0f
#define	RAY_WIDTH		0.05f
#define	IMPACT_WIDTH	0.2f
#define	NUM_RAYS		1000

//should match CommonFunctions.hlsli
#define	MAX_BONES		55

//data to store test rays
static vec3	sRayStarts[NUM_RAYS];
static vec3	sRayEnds[NUM_RAYS];
static vec4	sRayResults[NUM_RAYS];
static vec3	sRayImpacts[NUM_RAYS];
static int	sNumRayImpacts;

//static forward decs
static void	TestManyRays(const Terrain *pTer);
static void	PrintRandomPointInTerrain(const Terrain *pTer);
static bool	TestOneRay(const Terrain *pTer, const GameCamera *pCam, const vec3 eyePos, vec3 hitPos, vec4 hitPlane);
static void	KeyTurnHandler(const SDL_Event *pEvt, float *pDeltaYaw, float *pDeltaPitch);
static bool	KeyMovementHandler(GameCamera *pCam, vec3 eyePos, const SDL_Event *pEvt);
static void	ReBuildManyRayPrims(PrimObject **ppMR, PrimObject **ppMI, GraphicsDevice *pGD);


int main(void)
{
	printf("DirectX on looney loonix!\n");

	GraphicsDevice	*pGD;

	GD_Init(&pGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_0);

	StuffKeeper	*pSK	=StuffKeeper_Create(pGD);
	if(pSK == NULL)
	{
		printf("Couldn't create StuffKeeper!\n");
		GD_Destroy(&pGD);
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
	ID3D11RasterizerState	*pRast	=GD_CreateRasterizerState(pGD, &rastDesc);

	//hotkey stuff
	bool	bDrawTerNodes		=false;
	bool	bDrawHit			=false;
	bool	bDrawManyRays		=false;
	bool	bDrawManyImpacts	=false;

	Terrain	*pTer	=Terrain_Create(pGD, "Blort", "Textures/Terrain/HeightMaps/MZCloud.png", 10, HEIGHT_SCALAR);

	//debugdraw quadtree boxes
	int		numBounds;
	vec3	*pMins, *pMaxs;
	Terrain_GetQuadTreeLeafBoxes(pTer, &pMins, &pMaxs, &numBounds);

	PrimObject	*pQTBoxes	=PF_CreateCubesFromBoundArray(pMins, pMaxs, numBounds, pGD);

	//test prims
	PrimObject	*pCube	=PF_CreateSphere(GLM_VEC3_ZERO, 0.5f, pGD);
	LightRay	*pLR	=CP_CreateLightRay(5.0f, 0.25f, pGD);
	AxisXYZ		*pAxis	=CP_CreateAxis(5.0f, 0.1f, pGD);

	//debug prims for the lotsa ray tests
	PrimObject	*pManyRays;
	PrimObject	*pManyImpacts;

	CBKeeper	*pCBK	=CBK_Create(pGD);

	PostProcess	*pPP	=PP_Create(pGD, pSK, pCBK);

	PP_MakePostTarget(pPP, pGD, "LinearColor", RESX, RESY, DXGI_FORMAT_R8G8B8A8_UNORM);
	PP_MakePostDepth(pPP, pGD, "LinearDepth", RESX, RESY, DXGI_FORMAT_D32_FLOAT);

	float	aspect	=(float)RESX / (float)RESY;

	mat4	charMat, hitSphereMat;
	vec3	eyePos	={ 0.0f, 0.6f, 4.5f };
	vec3	targPos	={ 0.0f, 0.75f, 0.0f };
	vec3	upVec	={ 0.0f, 1.0f, 0.0f };

	//collision debuggery
	vec3	hitPos;
	vec4	hitPlane;
	bool	bHit;

	GameCamera	*pCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 30.0f);

	//projection won't change in this test program
	{
		mat4	proj;
		GameCam_GetProjection(pCam, proj);
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

	GD_RSSetViewPort(pGD, &vp);

	//set constant buffers to shaders, think I just have to do this once
	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "SkinWNormWPosTex0VS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));	
	CBK_SetCommonCBToShaders(pCBK, pGD);

	//for each set of shaders used?
	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexFactVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTexFact8PS"));	
	CBK_SetCommonCBToShaders(pCBK, pGD);

	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));	
	CBK_SetCommonCBToShaders(pCBK, pGD);

	GD_RSSetState(pGD, pRast);
	GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
	GD_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	vec3	lightDir	={	0.3f, -0.7f, -0.5f	};
	vec3	skyGrad0	={	0.7f, 0.2f, 0.1f	};
	vec3	skyGrad1	={	0.2f, 0.3f, 1.0f	};
	vec3	dangly		={	700.0f, 0.0f, 0.0f	};

	glm_vec3_normalize(lightDir);
	glm_mat4_identity(charMat);

	CBK_SetFogVars(pCBK, 500.0f, 1000.0f, true);
	CBK_SetSky(pCBK, skyGrad0, skyGrad1);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);

	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

	//character
	Mesh		*pMesh	=Mesh_Read(pGD, pSK, "Characters/Body.mesh");
	Character	*pChar	=Character_Read("Characters/DocuBlender.Character");
	AnimLib		*pALib	=AnimLib_Read("Characters/DocuBlender.AnimLib");

	mat4	bones[MAX_BONES];

	float	animTime	=0.0f;

	bool	bRunning	=true;
	while(bRunning)
	{
		float	deltaYaw, deltaPitch;

		UpdateTimer_Stamp(pUT);
		while(UpdateTimer_GetUpdateDeltaSeconds(pUT) > 0.0f)
		{
			vec3	forward, right, up;

			GameCam_GetForwardVec(pCam, forward);
			GameCam_GetRightVec(pCam, right);
			GameCam_GetUpVec(pCam, up);

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
			}

			//do input here
			//move turn etc

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
		GD_OMSetBlendState(pGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
		GD_PSSetSampler(pGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

		//camera update
		GameCam_Update(pCam, eyePos, deltaPitch, deltaYaw, 0.0f);
		deltaYaw	=deltaPitch	=0.0f;

		//set CB view
		{
			vec3	zeroVec		={	0.0f, 0.0f, 0.0f		};
			vec4	idntQuat	={	0.0f, 0.0f, 0.0f, 1.0f	};
			mat4	viewMat;
			vec4	viewQuat;
			vec3	negPos;

			GameCam_GetViewMatrixFly(pCam, viewMat, eyePos, viewQuat);

			glm_vec3_negate_to(eyePos, negPos);

			CBK_SetView(pCBK, viewMat, negPos);
		}


		PP_SetTargets(pPP, pGD, "LinearColor", "LinearDepth");

		GD_OMSetDepthStencilState(pGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		PP_ClearDepth(pPP, pGD, "LinearDepth");
		PP_ClearTarget(pPP, pGD, "LinearColor");

		//set input layout for the prim draws
		GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
		GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
		GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriSolidSpecPS"));

		//update frame CB
		CBK_UpdateFrame(pCBK, pGD);

		//draw light ray
		GD_PSSetSRV(pGD, NULL, 0);
		CP_DrawLightRay(pLR, lightDir, lightRayCol, pCBK, pGD);

		//draw xyz axis
		CP_DrawAxis(pAxis, lightDir, XAxisCol, YAxisCol, ZAxisCol, pCBK, pGD);

		//impact sphere VB/IB etc
		if(bDrawHit)
		{
			GD_IASetVertexBuffers(pGD, pCube->mpVB, 24, 0);
			GD_IASetIndexBuffers(pGD, pCube->mpIB, DXGI_FORMAT_R16_UINT, 0);
			GD_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, "Floors/Floor13"), 0);
			GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));
			CBK_SetSpecular(pCBK, specColor, 6.0f);
			CBK_SetTrilights3(pCBK, light0, light1, light2, lightDir);
			CBK_SetSolidColour(pCBK, solidColor0);

			CBK_SetWorldMat(pCBK, hitSphereMat);
			CBK_SetSolidColour(pCBK, solidColor0);
			CBK_UpdateObject(pCBK, pGD);
			GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);
		}

		//debug draw quadtree leaf cubes
		if(bDrawTerNodes)
		{
			GD_IASetVertexBuffers(pGD, pQTBoxes->mpVB, 24, 0);
			GD_IASetIndexBuffers(pGD, pQTBoxes->mpIB, DXGI_FORMAT_R32_UINT, 0);
			CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
			CBK_UpdateObject(pCBK, pGD);
			GD_DrawIndexed(pGD, pQTBoxes->mIndexCount, 0, 0);
		}

		//debug draw many rays
		if(bDrawManyRays && pManyRays != NULL)
		{
			GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormCol0"));
			GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosVColorVS"));
			GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriSolidVColorSpecPS"));

			GD_IASetVertexBuffers(pGD, pManyRays->mpVB, 28, 0);
			GD_IASetIndexBuffers(pGD, pManyRays->mpIB, DXGI_FORMAT_R32_UINT, 0);
			CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
			CBK_UpdateObject(pCBK, pGD);
			GD_DrawIndexed(pGD, pManyRays->mIndexCount, 0, 0);
		}

		if(bDrawManyImpacts && sNumRayImpacts > 0)
		{
			GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormCol0"));
			GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosVColorVS"));
			GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriSolidVColorSpecPS"));

			GD_IASetVertexBuffers(pGD, pManyImpacts->mpVB, 28, 0);
			GD_IASetIndexBuffers(pGD, pManyImpacts->mpIB, DXGI_FORMAT_R32_UINT, 0);
			CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
			CBK_UpdateObject(pCBK, pGD);
			GD_DrawIndexed(pGD, pManyImpacts->mIndexCount, 0, 0);
		}

		//set up terrain draw
		CBK_SetWorldMat(pCBK, GLM_MAT4_IDENTITY);
		CBK_UpdateObject(pCBK, pGD);
		Terrain_Draw(pTer, pGD, pSK);

		//set mesh draw stuff
		CBK_SetWorldMat(pCBK, charMat);
		CBK_SetDanglyForce(pCBK, dangly);
		CBK_SetSolidColour(pCBK, solidColor0);
		CBK_UpdateObject(pCBK, pGD);
		GD_PSSetSampler(pGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

		//bones
		CBK_SetBonesWithTranspose(pCBK, bones);
		CBK_UpdateCharacter(pCBK, pGD);
		CBK_SetCharacterToShaders(pCBK, pGD);

		//draw mesh
		Mesh_Draw(pMesh, pGD, pSK, "SkinWNormWPosTex0VS", "TriTex0SpecPS", "Characters/Docu");

		PP_ClearDepth(pPP, pGD, "BackDepth");
		PP_SetTargets(pPP, pGD, "BackColor", "BackDepth");

		PP_SetSRV(pPP, pGD, "LinearColor", 1);	//1 for colortex

		PP_DrawStage(pPP, pGD, pCBK);

		GD_Present(pGD);
	}

	GD_Destroy(&pGD);

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
			*pDeltaYaw	=KEYTURN_RATE;
		}
		else if(pEvt->key.keysym.sym == SDLK_e)
		{
			*pDeltaYaw	=-KEYTURN_RATE;
		}
		else if(pEvt->key.keysym.sym == SDLK_r)
		{
			*pDeltaPitch	=KEYTURN_RATE;
		}
		else if(pEvt->key.keysym.sym == SDLK_t)
		{
			*pDeltaPitch	=-KEYTURN_RATE;
		}
	}
}