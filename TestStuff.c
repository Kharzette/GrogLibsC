#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<x86intrin.h>
#include	<SDL3/SDL.h>
#include	<cglm/call.h>
//#include	"AudioLib/Audio.h"	audio stuff not ready yet
#include	"MaterialLib/StuffKeeper.h"
#include	"MaterialLib/CBKeeper.h"
#include	"MaterialLib/PostProcess.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/PrimFactory.h"
#include	"MeshLib/Mesh.h"

#define	RESX			800
#define	RESY			600
#define	ROT_RATE		10.0f
#define	UVSCALE_RATE	1.0f


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

	GD_Init(&pGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_1);

	StuffKeeper	*pSK	=StuffKeeper_Create(pGD);

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

	PrimObject	*pCube	=PF_CreateCube(5.0f, pGD);
	CBKeeper	*pCBK	=CBK_Create(pGD);

	PostProcess	*pPP	=PP_Create(pGD, pSK, pCBK);

	PP_MakePostTarget(pPP, pGD, "LinearColor", RESX, RESY, DXGI_FORMAT_R8G8B8A8_UNORM);
	PP_MakePostDepth(pPP, pGD, "LinearDepth", RESX, RESY, DXGI_FORMAT_D32_FLOAT);

	float	aspect	=(float)RESX / (float)RESY;

	mat4	ident, world, view, proj, yaw, pitch, temp, meshMat;
	mat4	bump0, bump1;	//translate world a bit
	vec3	eyePos	={ 9.0f, 45.0f, 79.0f };
	vec3	targPos	={ 0.0f, 30.0f, 0.0f };
	vec3	upVec	={ 0.0f, 1.0f, 0.0f };

	//draw 2 more cubes
	vec3	bumpVec0	={ 12.0f, -2.0f, 0.0f };
	vec3	bumpVec1	={ -12.0f, -2.0f, 0.0f };

//	glmc_ortho_default(aspect, proj);
	glmc_perspective_default(aspect, proj);

	glm_lookat_rh(eyePos, targPos, upVec, view);

	glmc_mat4_identity(ident);
	glmc_mat4_identity(world);

	CBK_SetWorldMat(pCBK, world);
	CBK_SetView(pCBK, view, eyePos);
	CBK_SetProjection(pCBK, proj);

	glmc_translate_make(bump0, bumpVec0);
	glmc_translate_make(bump1, bumpVec1);

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

	GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
	GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));
	GD_RSSetState(pGD, pRast);
	GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
	GD_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	vec4	specColor	={	1.0f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor0	={	1.0f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor1	={	0.5f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor2	={	1.0f, 0.5f, 1.0f, 1.0f	};
	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};
	vec3	lightDir	={	0.3f, -0.7f, -0.5f	};

	glmc_vec3_normalize(lightDir);

	CBK_SetSpecular(pCBK, specColor, 6.0f);
	CBK_SetTrilights3(pCBK, light0, light1, light2, lightDir);
	CBK_SetSolidColour(pCBK, solidColor0);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, false);

	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

	float	cubeYaw		=0.0f;
	float	cubePitch	=0.0f;
	vec3	dangly		={	700.0f, 0.0f, 0.0f	};

	Mesh	*pMesh	=Mesh_Read(pGD, pSK, "Characters/Body.mesh");

	mat4	*bones	=malloc(sizeof(mat4) * 55);

	for(int i=0;i < 55;i++)
	{
		glmc_mat4_identity(bones[i]);
	}

	CBK_SetBones(pCBK, bones);
	CBK_UpdateCharacter(pCBK, pGD);

	glmc_rotate_y(ident, CGLM_PI, meshMat);

	bool	bRunning	=true;
	while(bRunning)
	{
		UpdateTimer_Stamp(pUT);
		while(UpdateTimer_GetUpdateDeltaSeconds(pUT) > 0.0f)
		{
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
				}
			}
			//do input here
			//move camera etc

			UpdateTimer_UpdateDone(pUT);
		}

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		GD_IASetVertexBuffers(pGD, pCube->mpVB, 24, 0);
		GD_IASetIndexBuffers(pGD, pCube->mpIB, DXGI_FORMAT_R16_UINT, 0);

		//set no blend, I think post processing turns it on maybe
		GD_OMSetBlendState(pGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
		GD_PSSetSampler(pGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

		SpinMatYawPitch(dt, world);
		CBK_SetWorldMat(pCBK, world);
		CBK_SetSolidColour(pCBK, solidColor0);

		//camera update

		//set CB view

		//update frame CB
		CBK_UpdateFrame(pCBK, pGD);

		//update per object
		CBK_UpdateObject(pCBK, pGD);

		//set constant buffers to shaders
		CBK_SetCommonCBToShaders(pCBK, pGD);


		PP_SetTargets(pPP, pGD, "LinearColor", "LinearDepth");

		GD_OMSetDepthStencilState(pGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));

		PP_ClearDepth(pPP, pGD, "LinearDepth");
		PP_ClearTarget(pPP, pGD, "LinearColor");

		//set input layout for the cube draw
		GD_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
		GD_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
		GD_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));
		GD_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, "RoughStone"), 0);

		GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);

		//draw another
		CBK_SetSolidColour(pCBK, solidColor1);
		glmc_mat4_mul(world, bump0, temp);
		CBK_SetWorldMat(pCBK, temp);
		CBK_UpdateObject(pCBK, pGD);
		GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);

		//and another
		CBK_SetSolidColour(pCBK, solidColor2);
		glmc_mat4_mul(world, bump1, temp);
		CBK_SetWorldMat(pCBK, temp);
		CBK_UpdateObject(pCBK, pGD);
		GD_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);

		//set mesh draw stuff
		SpinMatYaw(dt, meshMat);
		CBK_SetWorldMat(pCBK, meshMat);
		CBK_SetDanglyForce(pCBK, dangly);
		CBK_SetSolidColour(pCBK, solidColor0);
		CBK_UpdateObject(pCBK, pGD);
		CBK_SetCharacterToShaders(pCBK, pGD);
//		GD_PSSetSampler(pGD, StuffKeeper_GetSamplerState(pSK, "PointClamp"), 0);

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