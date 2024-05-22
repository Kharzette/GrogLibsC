#pragma	once
#include	<stdint.h>
#include	<stdbool.h>
#include	<cglm/call.h>
#include	<d3dcommon.h>
#include	<d3d11.h>

typedef struct GraphicsDevice_t	GraphicsDevice;


extern bool	GD_Init(GraphicsDevice **ppGD, const char *pWindowTitle,
			int posX, int posY, int w, int h, bool bRandomPos,
			D3D_FEATURE_LEVEL desiredFeatureLevel);

extern void	GD_Destroy(GraphicsDevice **ppGD);

//gets
extern int	GD_GetWidth(const GraphicsDevice *pGD);
extern int	GD_GetHeight(const GraphicsDevice *pGD);
extern int	GD_GetPosX(const GraphicsDevice *pGD);
extern int	GD_GetPosY(const GraphicsDevice *pGD);

extern D3D_FEATURE_LEVEL		GD_GetFeatureLevel(const GraphicsDevice *pGD);
extern ID3D11RenderTargetView	*GD_GetBackBufferView(const GraphicsDevice *pGD);
extern ID3D11DepthStencilView	*GD_GetDepthView(const GraphicsDevice *pGD);

//resource creation
extern ID3D11Texture2D	*GD_MakeTexture(GraphicsDevice *pGD,
							uint8_t **pRows, int w, int h, int rowPitch);
extern ID3D11Buffer	*GD_CreateBufferWithData(
	GraphicsDevice			*pGD,
	const D3D11_BUFFER_DESC	*pDesc,
	const void				*pData,
	size_t					dataSize);
extern ID3D11Buffer	*GD_CreateBuffer(
	GraphicsDevice			*pGD,
	const D3D11_BUFFER_DESC	*pDesc);
extern ID3D11Texture2D	*GD_CreateTexture(GraphicsDevice *pGD, const D3D11_TEXTURE2D_DESC *pDesc);
extern ID3D11RenderTargetView	*GD_CreateRenderTargetView(GraphicsDevice *pGD, ID3D11Resource *pRes, DXGI_FORMAT fmt);
extern ID3D11DepthStencilView	*GD_CreateDepthStencilView(GraphicsDevice *pGD, ID3D11Resource *pRes, DXGI_FORMAT fmt);

//shader creation stuff
extern ID3D11VertexShader	*GD_CreateVertexShader(GraphicsDevice *pGD,
						const uint8_t	*pCodeBytes, int codeLen);
extern ID3D11PixelShader	*GD_CreatePixelShader(GraphicsDevice *pGD,
						const uint8_t	*pCodeBytes, int codeLen);
extern ID3D11ShaderResourceView	*GD_CreateSRV(GraphicsDevice *pGD, ID3D11Resource *pRes, DXGI_FORMAT fmt);
extern ID3D11InputLayout	*GD_CreateInputLayout(
	GraphicsDevice					*pGD,
	const D3D11_INPUT_ELEMENT_DESC	*pIEDs,
	int								numIEDs,
	const void						*byteCode,
	size_t							codeLen);

//renderstate creation stuff
extern ID3D11RasterizerState	*GD_CreateRasterizerState(
		GraphicsDevice *pGD,	const D3D11_RASTERIZER_DESC 		*pDesc);
extern ID3D11SamplerState		*GD_CreateSamplerState(
		GraphicsDevice	*pGD,	const D3D11_SAMPLER_DESC			*pDesc);
extern ID3D11BlendState			*GD_CreateBlendState(
		GraphicsDevice	*pGD,	const D3D11_BLEND_DESC			*pDesc);
extern ID3D11DepthStencilState	*GD_CreateDepthStencilState(
		GraphicsDevice	*pGD,	const D3D11_DEPTH_STENCIL_DESC	*pDesc);

//do stuff
extern void GD_Draw(GraphicsDevice *pGD, uint32_t vertCount, uint32_t startVert);
extern void GD_DrawIndexed(GraphicsDevice *pGD,
	uint32_t indexCount, uint32_t startIndex, uint32_t baseVert);
extern void GD_Present(GraphicsDevice *pGD);

//set states
extern void	GD_SetWindowBordered(GraphicsDevice *pGD, bool bOn);
extern void GD_OMSetBlendState(GraphicsDevice *pGD, ID3D11BlendState *pBlend);
extern void GD_OMSetDepthStencilState(GraphicsDevice *pGD, ID3D11DepthStencilState *pDSS);
extern void GD_ClearDepthStencilView(GraphicsDevice *pGD, ID3D11DepthStencilView *pView);
extern void GD_IASetInputLayout(GraphicsDevice *pGD, ID3D11InputLayout *pLay);
extern void GD_IASetPrimitiveTopology(GraphicsDevice *pGD, D3D11_PRIMITIVE_TOPOLOGY top);
extern void GD_VSSetShader(GraphicsDevice *pGD, ID3D11VertexShader *pVS);
extern void GD_VSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf);
extern void GD_PSSetShader(GraphicsDevice *pGD, ID3D11PixelShader *pPS);
extern void GD_PSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf);
extern void	GD_PSSetSampler(GraphicsDevice *pGD, ID3D11SamplerState *pSamp, uint32_t slot);
extern void GD_RSSetState(GraphicsDevice *pGD, ID3D11RasterizerState *pRS);
extern void GD_RSSetViewPort(GraphicsDevice *pGD, const D3D11_VIEWPORT *pVP);
extern void GD_PSSetSRV(GraphicsDevice *pGD, ID3D11ShaderResourceView *pSRV, int slot);
extern void GD_IASetVertexBuffers(GraphicsDevice *pGD,
	ID3D11Buffer *pVB, uint32_t stride, uint32_t offset);
extern void GD_IASetIndexBuffers(GraphicsDevice *pGD,
	ID3D11Buffer *pIB, DXGI_FORMAT fmt, uint32_t offset);
extern void	GD_UpdateSubResource(GraphicsDevice *pGD,
	ID3D11Resource *pDest, const void *pSrcData);
extern void GD_ClearRenderTargetView(GraphicsDevice *pGD,
	ID3D11RenderTargetView *pView, vec4 clearColor);
extern void GD_OMSetRenderTargets(GraphicsDevice *pGD,
	ID3D11RenderTargetView *pRTV, ID3D11DepthStencilView *pDSV);
extern void	GD_MapDiscard(GraphicsDevice *pGD, ID3D11Resource *pRS, D3D11_MAPPED_SUBRESOURCE *pMSR);
extern void	GD_UnMap(GraphicsDevice *pGD, ID3D11Resource *pRS);