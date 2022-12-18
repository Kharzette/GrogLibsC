#pragma	once
#include	<stdint.h>
#include	<d3dcommon.h>
#include	<d3d11.h>

typedef struct GraphicsDevice_t	GraphicsDevice;


extern bool	GraphicsDevice_Init(GraphicsDevice **ppGD, const char *pWindowTitle, int w, int h, D3D_FEATURE_LEVEL desiredFeatureLevel);
extern void	GraphicsDevice_Destroy(GraphicsDevice **ppGD);

extern D3D_FEATURE_LEVEL	GraphicsDevice_GetFeatureLevel(GraphicsDevice *pGD);

//resource creation
extern ID3D11Texture2D	*GraphicsDevice_MakeTexture(GraphicsDevice *pGD,
							uint8_t **pRows, int w, int h, int rowPitch);

//shader creation stuff
extern ID3D11VertexShader	*GraphicsDevice_CreateVertexShader(GraphicsDevice *pGD,
						uint8_t	*pCodeBytes, int codeLen);
extern ID3D11VertexShader	*GraphicsDevice_CreatePixelShader(GraphicsDevice *pGD,
						uint8_t	*pCodeBytes, int codeLen);
extern ID3D11ShaderResourceView	*GraphicsDevice_CreateSRV(GraphicsDevice *pGD, ID3D11Resource *pRes);

//renderstate creation stuff
extern ID3D11RasterizerState	*GraphicsDevice_CreateRasterizerState(
		GraphicsDevice *pGD,	D3D11_RASTERIZER_DESC *pDesc);
extern ID3D11SamplerState		*GraphicsDevice_CreateSamplerState(
		GraphicsDevice	*pGD,	D3D11_SAMPLER_DESC	*pDesc);
extern ID3D11BlendState			*GraphicsDevice_CreateBlendState(
		GraphicsDevice	*pGD,	D3D11_BLEND_DESC	*pDesc);
extern ID3D11DepthStencilState	*GraphicsDevice_CreateDepthStencilState(
		GraphicsDevice	*pGD,	D3D11_DEPTH_STENCIL_DESC	*pDesc);

//set states
extern void GraphicsDevice_OMSetBlendState(GraphicsDevice *pGD, ID3D11BlendState *pBlend);