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

//should match CommonFunctions.hlsli
#define	MAX_BONES			55


static void SpinMatYawPitch(float dt, mat4 outWorld)
{
	static	mat4	ident, yaw, pitch;
	static	float	cubeYaw		=0.0f;
	static	float	cubePitch	=0.0f;

	glmc_mat4_identity(ident);

	cubeYaw		+=ROT_RATE * dt;
	cubePitch	+=0.25f * ROT_RATE * dt;

	//wrap angles
	cubeYaw		=fmodf(cubeYaw, 360.0f);
	cubePitch	=fmodf(cubePitch, 360.0f);

	glmc_rotate_y(ident, glm_rad(cubeYaw), yaw);
	glmc_rotate_x(ident, glm_rad(cubePitch), pitch);

	glmc_mat4_mul(yaw, pitch, outWorld);
}

static void SpinMatYaw(float dt, mat4 outWorld)
{
	static	mat4	ident;
	static	float	cubeYaw		=0.0f;

	glmc_mat4_identity(ident);

	cubeYaw		+=ROT_RATE * dt;

	//wrap angles
	cubeYaw		=fmodf(cubeYaw, 360.0f);

	glmc_rotate_y(ident, glm_rad(cubeYaw), outWorld);
}


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

	Terrain	*pTer	=Terrain_Create(pGD, "Blort", "Textures/Terrain/HeightMaps/MZCloud.png", 10, HEIGHT_SCALAR);

	//debugdraw quadtree boxes
	int		numBounds;
	vec3	*pMins, *pMaxs;
	Terrain_GetQuadTreeLeafBoxes(pTer, &pMins, &pMaxs, &numBounds);

	PrimObject	*pQTBoxes	=PF_CreateCubesFromBoundArray(pMins, pMaxs, numBounds, pGD);

	vec3	zeroVec;
	glm_vec3_zero(zeroVec);

	//test prims
//	PrimObject	*pCube	=PF_CreateCube(0.5f, pGD);
	PrimObject	*pCube	=PF_CreateSphere(zeroVec, 1.0f, pGD);
//	PrimObject	*pCube	=PF_CreateCapsule(0.5f, 2.0f, pGD);

	LightRay	*pLR	=CP_CreateLightRay(5.0f, 0.25f, pGD);
	AxisXYZ		*pAxis	=CP_CreateAxis(5.0f, 0.1f, pGD);

	CBKeeper	*pCBK	=CBK_Create(pGD);

	PostProcess	*pPP	=PP_Create(pGD, pSK, pCBK);

	PP_MakePostTarget(pPP, pGD, "LinearColor", RESX, RESY, DXGI_FORMAT_R8G8B8A8_UNORM);
	PP_MakePostDepth(pPP, pGD, "LinearDepth", RESX, RESY, DXGI_FORMAT_D32_FLOAT);

	float	aspect	=(float)RESX / (float)RESY;

	mat4	ident, world, yaw, pitch, temp, meshMat;
	mat4	bump0, bump1;	//translate world a bit
	vec3	eyePos	={ 0.0f, 0.6f, 4.5f };
	vec3	targPos	={ 0.0f, 0.75f, 0.0f };
	vec3	upVec	={ 0.0f, 1.0f, 0.0f };

	//collision debuggery
	vec3	hitPos, hitNorm;
	bool	bHit;

	GameCamera	*pCam	=GameCam_Create(false, 0.1f, 2000.0f, GLM_PI_4f, aspect, 1.0f, 30.0f);

	glmc_mat4_identity(ident);
	glmc_mat4_identity(world);

	CBK_SetWorldMat(pCBK, world);

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

	glmc_vec3_normalize(lightDir);

	CBK_SetFogVars(pCBK, 500.0f, 1000.0f, true);
	CBK_SetSky(pCBK, skyGrad0, skyGrad1);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);

	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

	float	cubeYaw		=0.0f;
	float	cubePitch	=0.0f;
	vec3	dangly		={	700.0f, 0.0f, 0.0f	};

	Mesh	*pMesh	=Mesh_Read(pGD, pSK, "Characters/Body.mesh");

//	glmc_rotate_y(ident, CGLM_PI, meshMat);
	glmc_mat4_identity(meshMat);

	Character	*pChar	=Character_Read("Characters/DocuBlender.Character");

	AnimLib	*pALib	=AnimLib_Read("Characters/DocuBlender.AnimLib");

	mat4	bones[MAX_BONES];

	float	animTime	=0.0f;

	bool	bRunning	=true;
	bool	bRotLight	=false;
	while(bRunning)
	{
		float	deltaYaw, deltaPitch;

		deltaYaw	=deltaPitch	=0.0f;

		UpdateTimer_Stamp(pUT);
		while(UpdateTimer_GetUpdateDeltaSeconds(pUT) > 0.0f)
		{
			vec3	forward, right, up;

			GameCam_GetForwardVec(pCam, forward);
			GameCam_GetRightVec(pCam, right);
			GameCam_GetUpVec(pCam, up);

			bRotLight	=false;

			SDL_Event	evt;
			while(SDL_PollEvent(&evt))
			{
				if(evt.type == SDL_QUIT)
				{
					bRunning	=false;
				}
				else if(evt.type == SDL_KEYDOWN)
				{
					if(evt.key.keysym.sym == SDLK_LEFT)
					{
						dangly[0]	-=UVSCALE_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_RIGHT)
					{
						dangly[0]	+=UVSCALE_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_w)
					{
						glm_vec3_sub(eyePos, forward, eyePos);
					}
					else if(evt.key.keysym.sym == SDLK_a)
					{
						glm_vec3_sub(eyePos, right, eyePos);
					}
					else if(evt.key.keysym.sym == SDLK_s)
					{
						glm_vec3_add(eyePos, forward, eyePos);
					}
					else if(evt.key.keysym.sym == SDLK_d)
					{
						glm_vec3_add(eyePos, right, eyePos);
					}
					else if(evt.key.keysym.sym == SDLK_z)
					{
						glm_vec3_sub(eyePos, up, eyePos);
					}
					else if(evt.key.keysym.sym == SDLK_c)
					{
						glm_vec3_add(eyePos, up, eyePos);
					}
					else if(evt.key.keysym.sym == SDLK_q)
					{
						deltaYaw	=KEYTURN_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_e)
					{
						deltaYaw	=-KEYTURN_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_r)
					{
						deltaPitch	=KEYTURN_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_t)
					{
						deltaPitch	=-KEYTURN_RATE;
					}
					else if(evt.key.keysym.sym == SDLK_ESCAPE)
					{
						bRunning	=false;
					}
					else if(evt.key.keysym.sym == SDLK_l)
					{
						bRotLight	=true;
					}
					else if(evt.key.keysym.sym == SDLK_i)
					{
						vec3	testMin		={-10, -10, -10};
						vec3	testMax		={-5, -5, -5};
						vec4	testPlane	={1, 0, 0, 10};
						vec3	endRay;

						vec4	testVol[6];
						MakeConvexVolumeFromBound(testMin, testMax, testVol);

						glm_vec3_scale(forward, -RAY_LEN, endRay);
						glm_vec3_add(eyePos, endRay, endRay);

						//invalidate impact point
						hitPos[0]	=hitPos[1]	=hitPos[2]	=FLT_MAX;

						int	hitRes	=Terrain_LineIntersect(pTer, eyePos, endRay, hitPos, hitNorm);
//						int	hitRes	=LineIntersectBounds(testMin, testMax, eyePos, endRay, hitPos, hitNorm);
//						int	hitRes	=LineIntersectPlane(testPlane, eyePos, endRay, hitPos);
//						int	hitRes	=LineIntersectVolume(testVol, 6, eyePos, endRay, hitPos, hitNorm);
						if(hitRes == VOL_MISS)
						{
							solidColor0[0]	=solidColor0[1]	=solidColor0[2]	=1.0f;
						}
						else if(hitRes == VOL_INSIDE)
						{
							glm_translate_make(world, hitPos);
							solidColor0[0]	=solidColor0[2]	=1.0f;
							solidColor0[1]	=0.0f;
						}
						else if(hitRes == VOL_HIT)
						{
							glm_translate_make(world, hitPos);
							solidColor0[0]	=solidColor0[1]	=1.0f;
							solidColor0[2]	=0.0f;
						}
						else if(hitRes == VOL_HIT_INSIDE)
						{
							glm_translate_make(world, hitPos);
							solidColor0[1]	=solidColor0[2]	=1.0f;
							solidColor0[0]	=0.0f;
						}
					}
				}
			}
			//do input here
			//move turn etc
			if(bRotLight)
			{
				glmc_vec3_rotate_m4(world, lightDir, lightDir);
				CBK_SetTrilights3(pCBK, light0, light1, light2, lightDir);
			}

			UpdateTimer_UpdateDone(pUT);
		}

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		animTime	+=dt;
//		AnimLib_Animate(pALib, "DocuBaseBlenderCoords", animTime);
//		AnimLib_Animate(pALib, "DocuWalkBlenderCoords", animTime);
		AnimLib_Animate(pALib, "DocuIdleBlenderCoords", animTime);
		
//		AnimLib_FillBoneArray(pALib, bones);
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

		//cube VB/IB etc
		GD_IASetVertexBuffers(pGD, pCube->mpVB, 24, 0);
		GD_IASetIndexBuffers(pGD, pCube->mpIB, DXGI_FORMAT_R16_UINT, 0);
		GD_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, "Floors/Floor13"), 0);
		GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));
		CBK_SetSpecular(pCBK, specColor, 6.0f);
		CBK_SetTrilights3(pCBK, light0, light1, light2, lightDir);
		CBK_SetSolidColour(pCBK, solidColor0);

//		SpinMatYawPitch(dt, world);
		CBK_SetWorldMat(pCBK, world);
		CBK_SetSolidColour(pCBK, solidColor0);
		CBK_UpdateObject(pCBK, pGD);
		GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);


		//draw another
//		CBK_SetSolidColour(pCBK, solidColor1);
//		glmc_mat4_mul(world, bump0, temp);
//		CBK_SetWorldMat(pCBK, temp);
//		CBK_UpdateObject(pCBK, pGD);
//		GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);

		//and another
//		CBK_SetSolidColour(pCBK, solidColor2);
//		glmc_mat4_mul(world, bump1, temp);
//		CBK_SetWorldMat(pCBK, temp);
//		CBK_UpdateObject(pCBK, pGD);
//		GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);

		//debug draw quadtree leaf cubes
		GD_IASetVertexBuffers(pGD, pQTBoxes->mpVB, 24, 0);
		GD_IASetIndexBuffers(pGD, pQTBoxes->mpIB, DXGI_FORMAT_R32_UINT, 0);
		CBK_SetWorldMat(pCBK, ident);
		CBK_UpdateObject(pCBK, pGD);
		GD_DrawIndexed(pGD, pQTBoxes->mIndexCount, 0, 0);
//		GD_DrawIndexed(pGD, 36 * 4, 0, 0);

		//set up terrain draw
		CBK_SetWorldMat(pCBK, ident);
		CBK_UpdateObject(pCBK, pGD);
		Terrain_Draw(pTer, pGD, pSK);

		//set mesh draw stuff
		SpinMatYaw(dt, meshMat);
		CBK_SetWorldMat(pCBK, meshMat);
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