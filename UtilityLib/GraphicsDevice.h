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
extern ID3D11Buffer	*GraphicsDevice_CreateBufferWithData(
	GraphicsDevice		*pGD,
	D3D11_BUFFER_DESC	*pDesc,
	const void			*pData,
	size_t				dataSize);
ID3D11Buffer	*GraphicsDevice_CreateBuffer(
	GraphicsDevice		*pGD,
	D3D11_BUFFER_DESC	*pDesc);

//shader creation stuff
extern ID3D11VertexShader	*GraphicsDevice_CreateVertexShader(GraphicsDevice *pGD,
						uint8_t	*pCodeBytes, int codeLen);
extern ID3D11VertexShader	*GraphicsDevice_CreatePixelShader(GraphicsDevice *pGD,
						uint8_t	*pCodeBytes, int codeLen);
extern ID3D11ShaderResourceView	*GraphicsDevice_CreateSRV(GraphicsDevice *pGD, ID3D11Resource *pRes);
extern ID3D11InputLayout	*GraphicsDevice_CreateInputLayout(
	GraphicsDevice				*pGD,
	D3D11_INPUT_ELEMENT_DESC	*pIEDs,
	int							numIEDs,
	const void					*byteCode,
	size_t						codeLen);

//renderstate creation stuff
extern ID3D11RasterizerState	*GraphicsDevice_CreateRasterizerState(
		GraphicsDevice *pGD,	D3D11_RASTERIZER_DESC 		*pDesc);
extern ID3D11SamplerState		*GraphicsDevice_CreateSamplerState(
		GraphicsDevice	*pGD,	D3D11_SAMPLER_DESC			*pDesc);
extern ID3D11BlendState			*GraphicsDevice_CreateBlendState(
		GraphicsDevice	*pGD,	D3D11_BLEND_DESC			*pDesc);
extern ID3D11DepthStencilState	*GraphicsDevice_CreateDepthStencilState(
		GraphicsDevice	*pGD,	D3D11_DEPTH_STENCIL_DESC	*pDesc);

//do stuff
extern void GraphicsDevice_Draw(GraphicsDevice *pGD, uint32_t vertCount, uint32_t startVert);
extern void GraphicsDevice_Present(GraphicsDevice *pGD);

//set states
extern void GraphicsDevice_OMSetBlendState(GraphicsDevice *pGD, ID3D11BlendState *pBlend);
extern void GraphicsDevice_OMSetDepthStencilState(GraphicsDevice *pGD, ID3D11DepthStencilState *pDSS);
extern void GraphicsDevice_ClearDepthStencilView(GraphicsDevice *pGD);
extern void GraphicsDevice_ClearRenderTargetView(GraphicsDevice *pGD, const float *pF4ClearColor);
extern void GraphicsDevice_OMSetRenderTargets(GraphicsDevice *pGD);
extern void GraphicsDevice_IASetInputLayout(GraphicsDevice *pGD, ID3D11InputLayout *pLay);
extern void GraphicsDevice_IASetPrimitiveTopology(GraphicsDevice *pGD, D3D11_PRIMITIVE_TOPOLOGY top);
extern void GraphicsDevice_VSSetShader(GraphicsDevice *pGD, ID3D11VertexShader *pVS);
extern void GraphicsDevice_VSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf);
extern void GraphicsDevice_PSSetShader(GraphicsDevice *pGD, ID3D11PixelShader *pPS);
extern void GraphicsDevice_PSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf);
extern void GraphicsDevice_RSSetState(GraphicsDevice *pGD, ID3D11RasterizerState *pRS);
extern void GraphicsDevice_PSSetSRV(GraphicsDevice *pGD, ID3D11ShaderResourceView *pSRV);
extern void GraphicsDevice_IASetVertexBuffers(GraphicsDevice *pGD,
	ID3D11Buffer *pVB, uint32_t stride, uint32_t offset);
extern void	GraphicsDevice_UpdateSubResource(GraphicsDevice *pGD,
	ID3D11Resource *pDest, const void *pSrcData);
