#include	<d3d11_1.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<SDL3/SDL.h>
//#include	"AudioLib/Audio.h"	audio stuff not ready yet
#include	"MaterialLib/StuffKeeper.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/DictionaryStuff.h"
#include	"UtilityLib/UpdateTimer.h"


//test struct
struct testJunx
{
	int		someVal;
	char	desc[32];
};

struct Vertex
{
	float x, y;
	
	//will have to fill these via conversion
	//as there's no 16 bit floats
	uint16_t	u, v;
};

static struct Vertex vertices[] =
{
	{  0.0f,  0.5f, 1.f, 0.f },
	{  0.5f, -0.5f, 0.f, 1.f },
	{ -0.5f, -0.5f, 0.f, 0.f },
};

uint16_t	ConvertToF16(float f32)
{
	uint32_t	fRaw	=*(uint32_t *)(&f32);
	//split
	bool		bSign		=fRaw >> 31;
	uint32_t	exponent	=fRaw >> 24;
	uint32_t	mantissa	=fRaw & 0x7FFFFF;

	uint16_t	ret	=bSign << 15 | exponent << 10 | mantissa;
}


int main(void)
{
	printf("DirectX on looney loonix!\n");

	GraphicsDevice	*pGD;

	GraphicsDevice_Init(&pGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_0);

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

	D3D11_BUFFER_DESC	vbDesc;
	vbDesc.BindFlags			=D3D11_BIND_VERTEX_BUFFER;
	vbDesc.ByteWidth			=sizeof(vertices);
	vbDesc.CPUAccessFlags		=DXGI_CPU_ACCESS_NONE;
	vbDesc.MiscFlags			=0;
	vbDesc.StructureByteStride	=0;
	vbDesc.Usage				=D3D11_USAGE_IMMUTABLE;

	//no half types!  wtf
	vertices[0].u	=ConvertToF16(1.0f);
	vertices[0].v	=ConvertToF16(0.0f);

	vertices[1].u	=ConvertToF16(0.0f);
	vertices[1].v	=ConvertToF16(1.0f);

	vertices[2].u	=ConvertToF16(0.0f);
	vertices[2].v	=ConvertToF16(0.0f);

	ID3D11Buffer	*pVB	=GraphicsDevice_CreateBufferWithData(pGD, &vbDesc, vertices, sizeof(vertices));

	//good old xna blue
	float	clearColor[]	={ 100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f };

	GraphicsDevice_IASetInputLayout(pGD, StuffKeeper_GetInputLayout(pSK, "VPosTex0"));
	GraphicsDevice_IASetVertexBuffers(pGD, pVB, 20, 0);
	GraphicsDevice_IASetPrimitiveTopology(pGD, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	GraphicsDevice_VSSetShader(pGD, StuffKeeper_GetVertexShader(pSK, "TextVS"));
	GraphicsDevice_PSSetShader(pGD, StuffKeeper_GetPixelShader(pSK, "TextPS"));
	GraphicsDevice_RSSetState(pGD, pRast);
	GraphicsDevice_OMSetBlendState(pGD, StuffKeeper_GetBlendState(pSK, "NoBlending"));
	GraphicsDevice_PSSetSRV(pGD, StuffKeeper_GetSRV(pSK, "Brick"));

	UpdateTimer	*pUT	=UpdateTimer_Create(true, true);

	UpdateTimer_SetFixedTimeStepMilliSeconds(pUT, 6.944444f);	//144hz

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
			UpdateTimer_UpdateDone(pUT);
		}

		//set constant buffers to shaders
		//camera update
		//set CB view
		//update frame CB

		//render update
		float	dt	=UpdateTimer_GetRenderUpdateDeltaSeconds(pUT);

		GraphicsDevice_OMSetRenderTargets(pGD);
		GraphicsDevice_OMSetDepthStencilState(pGD, StuffKeeper_GetDepthStencilState(pSK, "EnableDepth"));
		GraphicsDevice_ClearDepthStencilView(pGD);

		GraphicsDevice_ClearRenderTargetView(pGD, clearColor);

		GraphicsDevice_Draw(pGD, 3, 0);
		GraphicsDevice_Present(pGD);
	}

	sleep(1);

	GraphicsDevice_Destroy(&pGD);

	return	EXIT_SUCCESS;
}