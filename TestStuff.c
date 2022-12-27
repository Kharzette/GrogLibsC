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
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"
#include	"UtilityLib/PrimFactory.h"

#define	RESX		800
#define	RESY		600
#define	ROT_RATE	10.0f


int main(void)
{
	printf("DirectX on looney loonix!\n");

	GraphicsDevice	*pGD;

	GraphicsDevice_Init(&pGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_1);

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
	ID3D11RasterizerState	*pRast	=GraphicsDevice_CreateRasterizerState(pGD, &rastDesc);

	PrimObject	*pCube	=PF_CreateCube(5.0f, pGD);
	CBKeeper	*pCBK	=CBK_Create(pGD);

	float	aspect	=(float)RESX / (float)RESY;

	mat4	ident, world, view, proj, yaw, pitch;
	vec3	eyePos	={ 9.0f, 10.0f, 22.0f };
	vec3	targPos	={ 0.0f, 0.0f, 0.0f };
	vec3	upVec	={ 0.0f, 1.0f, 0.0f };

//	glmc_ortho_default(aspect, proj);
	glmc_perspective_default(aspect, proj);

	glm_lookat_rh(eyePos, targPos, upVec, view);

	glmc_mat4_identity(ident);
	glmc_mat4_identity(world);

	CBK_SetWorldMat(pCBK, world);
	CBK_SetView(pCBK, view, eyePos);
	CBK_SetProjection(pCBK, proj);

	//good old xna blue
	float	clearColor[]	={ 100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f };

	D3D11_VIEWPORT	vp;

	vp.Width	=RESX;
	vp.Height	=RESY;
	vp.MaxDepth	=1.0f;
	vp.MinDepth	=0.0f;
	vp.TopLeftX	=0;
	vp.TopLeftY	=0;

	GraphicsDevice_RSSetViewPort(pGD, &vp);

	GraphicsDevice_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "WNormWPosTexVS"));
//	GraphicsDevice_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriSolidPS"));
	GraphicsDevice_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TriTex0SpecPS"));
	GraphicsDevice_RSSetState(pGD, pRast);
	GraphicsDevice_OMSetBlendState(pGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
	GraphicsDevice_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, "Brick"));
	GraphicsDevice_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosNormTex0"));
	GraphicsDevice_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	GraphicsDevice_PSSetSampler(pGD, StuffKeeper_GetSamplerState(pSK, "PointWrap"), 0);

	vec4	specColor	={	1.0f, 1.0f, 1.0f, 1.0f	};
	vec4	solidColor	={	1.0f, 1.0f, 1.0f, 1.0f	};
	vec3	light0		={	1.0f, 1.0f, 1.0f	};
	vec3	light1		={	0.2f, 0.3f, 0.3f	};
	vec3	light2		={	0.1f, 0.2f, 0.2f	};
	vec3	lightDir	={	0.3f, -0.7f, 0.2f	};

	glmc_vec3_normalize(lightDir);

	CBK_SetSpecular(pCBK, specColor, 6.0f);
	CBK_SetTrilights3(pCBK, light0, light1, light2, lightDir);
	CBK_SetSolidColour(pCBK, solidColor);

	UpdateTimer	*pUT	=UpdateTimer_Create(true, true);

	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

	float	cubeYaw		=0.0f;
	float	cubePitch	=0.0f;

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
			}
			//do input here
			//move camera etc

			cubeYaw		+=ROT_RATE * UpdateTimer_GetUpdateDeltaSeconds(pUT);
			cubePitch	+=0.25f * ROT_RATE * UpdateTimer_GetUpdateDeltaSeconds(pUT);

			//wrap angles
			cubeYaw		=fmodf(cubeYaw, 360.0f);
			cubePitch	=fmodf(cubePitch, 360.0f);

			glmc_rotate_y(ident, glm_rad(cubeYaw), yaw);
			glmc_rotate_x(ident, glm_rad(cubePitch), pitch);

			glmc_mat4_mul(yaw, pitch, world);

			CBK_SetWorldMat(pCBK, world);

			UpdateTimer_UpdateDone(pUT);
		}

		GraphicsDevice_IASetVertexBuffers(pGD, pCube->mpVB, 24, 0);
		GraphicsDevice_IASetIndexBuffers(pGD, pCube->mpIB, DXGI_FORMAT_R16_UINT, 0);

		CBK_SetWorldMat(pCBK, world);

		//camera update

		//set CB view

		//update frame CB
		CBK_UpdateFrame(pCBK, pGD);

		//update per object
		CBK_UpdateObject(pCBK, pGD);

		//set constant buffers to shaders
		CBK_SetCommonCBToShaders(pCBK, pGD);

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		GraphicsDevice_OMSetRenderTargets(pGD);
		GraphicsDevice_OMSetDepthStencilState(pGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));
		GraphicsDevice_ClearDepthStencilView(pGD);

		GraphicsDevice_ClearRenderTargetView(pGD, clearColor);

//		GraphicsDevice_Draw(pGD, pCube->mVertCount, 0);
		GraphicsDevice_DrawIndexed(pGD, pCube->mIndexCount, 0, 0);
		GraphicsDevice_Present(pGD);
	}

	GraphicsDevice_Destroy(&pGD);

	return	EXIT_SUCCESS;
}