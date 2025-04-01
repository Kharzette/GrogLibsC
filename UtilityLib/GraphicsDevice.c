#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<cglm/call.h>
#include	<stdio.h>
#include	"GraphicsDevice.h"

#define	D3D11_NO_HELPERS
#define	CINTERFACE
#define	COBJMACROS


typedef struct GraphicsDevice_t
{
	//D3D interfaces
	SDL_Window				*mpWnd;
	ID3D11Device			*mpDevice;
	ID3D11Device1			*mpDevice1;
	ID3D11Debug				*mpDebug;
	ID3D11DeviceContext		*mpContext;
	ID3D11DeviceContext1	*mpContext1;
	IDXGISwapChain			*mpSwapChain;
	ID3D11RenderTargetView	*mpBackBufferView;
	ID3D11DepthStencilView	*mpDepthBufferView;

	//featurelevel the device was created with
	D3D_FEATURE_LEVEL	mFeatureLevel;

	//size
	int	mWidth, mHeight;
}	GraphicsDevice;


//try to make a device of the desired feature level
//TODO: get the swap effect stuff right
bool	GD_Init(GraphicsDevice **ppGD, const char *pWindowTitle,
			int posX, int posY, int w, int h, bool bRandomPos,
			D3D_FEATURE_LEVEL desiredFeatureLevel)
{
	//alloc
	*ppGD	=malloc(sizeof(GraphicsDevice));
	memset(*ppGD, 0, sizeof(GraphicsDevice));

	GraphicsDevice	*pGD	=*ppGD;

	pGD->mWidth		=w;
	pGD->mHeight	=h;

	int	res	=SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
	if(res < 0)
	{
		printf("SDL init failed: %s\n", SDL_GetError());
		return	FALSE;
	}

	SDL_Vulkan_LoadLibrary(NULL);

	SDL_PropertiesID	props	=SDL_CreateProperties();

	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, pWindowTitle);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, w);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, h);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);

	if(bRandomPos)
	{	
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_UNDEFINED);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_UNDEFINED);
	}
	else
	{
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, posX);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, posY);
	}

	pGD->mpWnd	=SDL_CreateWindowWithProperties(props);

	if(pGD->mpWnd == NULL)
	{
		printf("Window creation didn't work: %s\n", SDL_GetError());
		SDL_Quit();
		return	FALSE;
	}

	D3D_FEATURE_LEVEL	featureLevels[]	={	desiredFeatureLevel	};

	DXGI_SWAP_CHAIN_DESC	scDesc	={};

	//important to note that dxvk native expects an SDL_Window in place
	//of an ordinary hwnd in most cases.  This wasn't very clear at first
	//and cost me hours of debugging!
	scDesc.BufferDesc.Format					=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	scDesc.BufferDesc.RefreshRate.Numerator		=60;
	scDesc.BufferDesc.RefreshRate.Denominator	=1;
	scDesc.SampleDesc.Count						=1;
	scDesc.SampleDesc.Quality					=0;
	scDesc.BufferUsage							=DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow							=pGD->mpWnd;	//Use SDL_Window here!
	scDesc.Windowed								=TRUE;
	scDesc.BufferCount							=1;
	scDesc.SwapEffect							=DXGI_SWAP_EFFECT_DISCARD;
	scDesc.Flags								=0;

	//device create go!
	HRESULT	hres	=D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		D3D11_CREATE_DEVICE_DEBUG, featureLevels, 1, D3D11_SDK_VERSION,
		&scDesc, &pGD->mpSwapChain, &pGD->mpDevice, &pGD->mFeatureLevel, &pGD->mpContext);
	if(hres != S_OK)
	{
		printf("Error creating device & chain: %X\n", hres);
		return	FALSE;
	}

	//grab a device 1
	hres	=pGD->mpDevice->lpVtbl->QueryInterface(pGD->mpDevice, &IID_ID3D11Device1, (void **)&pGD->mpDevice1);
	if(hres != S_OK)
	{
		printf("Error Querifying Device1: %X\n", hres);
		goto	quit;
	}

	//grab a DC1
	hres	=pGD->mpContext->lpVtbl->QueryInterface(pGD->mpContext,
				&IID_ID3D11DeviceContext1,
				(void **)&pGD->mpContext1);
	if(hres != S_OK)
	{
		printf("Error Querifying context1: %X\n", hres);
		goto	quit;
	}
/*
	IDXGIFactory	*pFact;

	hres	=CreateDXGIFactory(&IID_IDXGIFactory, (void **)&pFact);
	if(hres != S_OK)
	{
		printf("Error creating dxgi factory: %X\n", hres);
		goto	quit;
	}

	IDXGIFactory2	*pFact2;

	hres	=pFact->lpVtbl->QueryInterface(pFact, &IID_IDXGIFactory2, (void **)&pFact2);
	if(hres != S_OK)
	{
		printf("Error creating dxgi factory2: %X\n", hres);
		goto	quit;
	}
*/

	//render target view
	ID3D11Texture2D	*backBuffer;
	hres	=pGD->mpSwapChain->lpVtbl->GetBuffer(pGD->mpSwapChain,
		0, &IID_ID3D11Texture2D, (void **)&backBuffer);
	if(hres != S_OK)
	{
		printf("Error getting back buffer: %X\n", hres);
		goto	quit;
	}

	pGD->mpBackBufferView	=GD_CreateRenderTargetView(pGD,
		(ID3D11Resource *)backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	//release backbuffer, just needed for this one call
	backBuffer->lpVtbl->Release(backBuffer);

	//create a depth buffer
	D3D11_TEXTURE2D_DESC	texDesc;

	texDesc.ArraySize			=1;
	texDesc.BindFlags			=D3D11_BIND_DEPTH_STENCIL;
	texDesc.CPUAccessFlags		=0;
	texDesc.MipLevels			=1;
	texDesc.MiscFlags			=0;
	texDesc.Usage				=D3D11_USAGE_DEFAULT;
	texDesc.Width				=w;
	texDesc.Height				=h;
	texDesc.Format				=DXGI_FORMAT_D32_FLOAT;
	texDesc.SampleDesc.Count	=1;
	texDesc.SampleDesc.Quality	=0;

	ID3D11Texture2D	*pDepthBuffer;
	hres	=pGD->mpDevice1->lpVtbl->CreateTexture2D(pGD->mpDevice1, &texDesc, NULL, &pDepthBuffer);
	if(hres != S_OK)
	{
		printf("Error creating depth buffer texture: %X\n", hres);
		goto	quit;
	}

	pGD->mpDepthBufferView	=GD_CreateDepthStencilView(pGD,
		(ID3D11Resource *)pDepthBuffer, texDesc.Format);

	//release depthTex, just needed for this one call
	pDepthBuffer->lpVtbl->Release(pDepthBuffer);

	return	TRUE;

quit:
	SDL_Vulkan_UnloadLibrary();

	SDL_DestroyWindow(pGD->mpWnd);

	SDL_Quit();

	return	FALSE;
}

D3D_FEATURE_LEVEL	GD_GetFeatureLevel(const GraphicsDevice *pGD)
{
	return	pGD->mFeatureLevel;
}

int	GD_GetWidth(const GraphicsDevice *pGD)
{
	return	pGD->mWidth;
}

int	GD_GetHeight(const GraphicsDevice *pGD)
{
	return	pGD->mHeight;
}

int	GD_GetPosX(const GraphicsDevice *pGD)
{
	int	x;
	SDL_GetWindowPosition(pGD->mpWnd, &x, NULL);

	return	x;
}

int	GD_GetPosY(const GraphicsDevice *pGD)
{
	int	y;
	SDL_GetWindowPosition(pGD->mpWnd, NULL, &y);

	return	y;
}

void	GD_SetWindowBordered(GraphicsDevice *pGD, bool bOn)
{
	SDL_SetWindowBordered(pGD->mpWnd, bOn);
}

void	GD_SetMouseRelative(GraphicsDevice *pGD, bool bOn)
{
	SDL_SetWindowRelativeMouseMode(pGD->mpWnd, bOn);
}

ID3D11RenderTargetView	*GD_GetBackBufferView(const GraphicsDevice *pGD)
{
	return	pGD->mpBackBufferView;
}

ID3D11DepthStencilView	*GD_GetDepthView(const GraphicsDevice *pGD)
{
	return	pGD->mpDepthBufferView;
}

void	GD_Destroy(GraphicsDevice **ppGD)
{
	GraphicsDevice	*pGD	=*ppGD;

	pGD->mpContext1->lpVtbl->Release(pGD->mpContext1);
	pGD->mpDevice1->lpVtbl->Release(pGD->mpDevice1);
	pGD->mpContext->lpVtbl->Release(pGD->mpContext);
	pGD->mpSwapChain->lpVtbl->Release(pGD->mpSwapChain);
	pGD->mpDevice->lpVtbl->Release(pGD->mpDevice);

	*ppGD	=NULL;

	SDL_Vulkan_UnloadLibrary();

	SDL_DestroyWindow(pGD->mpWnd);

	SDL_Quit();
}

ID3D11DepthStencilView	*GD_CreateDepthStencilView(GraphicsDevice *pGD,
	ID3D11Resource *pRes, DXGI_FORMAT fmt)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC	depthDesc;
	memset(&depthDesc, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

	depthDesc.Format		=fmt;
	depthDesc.ViewDimension	=D3D11_DSV_DIMENSION_TEXTURE2D;

	ID3D11DepthStencilView	*pRet;

	HRESULT	hres	=pGD->mpDevice1->lpVtbl->CreateDepthStencilView(pGD->mpDevice1,
		pRes, &depthDesc, &pRet);
	if(hres != S_OK)
	{
		printf("Error creating depth buffer view: %X\n", hres);
		return	NULL;
	}
	return	pRet;
}

ID3D11RenderTargetView	*GD_CreateRenderTargetView(GraphicsDevice *pGD, ID3D11Resource *pRes, DXGI_FORMAT fmt)
{
	D3D11_RENDER_TARGET_VIEW_DESC	rtvDesc;
	memset(&rtvDesc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));

	rtvDesc.ViewDimension		=D3D11_SRV_DIMENSION_TEXTURE2D;
	rtvDesc.Format				=fmt;

	ID3D11RenderTargetView	*pRet;
	
	HRESULT	hres	=pGD->mpDevice1->lpVtbl->CreateRenderTargetView(pGD->mpDevice1,
		pRes, NULL, &pRet);

	if(hres != S_OK)
	{
		printf("Error making render target view: %X\n", hres);
		return	NULL;
	}
	return	pRet;
}

//no initial data
ID3D11Texture2D	*GD_CreateTexture(GraphicsDevice *pGD, const D3D11_TEXTURE2D_DESC *pDesc)
{
	ID3D11Texture2D	*pTex;
	HRESULT	hres	=pGD->mpDevice1->lpVtbl->CreateTexture2D(pGD->mpDevice1, pDesc, NULL, &pTex);
	if(hres != S_OK)
	{
		printf("Error creating texture: %X\n", hres);
		return	NULL;
	}
	return	pTex;
}

ID3D11Texture2D	*GD_MakeTexture(GraphicsDevice *pGD, uint8_t **pRows, int w, int h, int rowPitch)
{
	D3D11_TEXTURE2D_DESC	texDesc;

	texDesc.ArraySize			=1;
	texDesc.BindFlags			=D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags		=0;
	texDesc.MipLevels			=1;
	texDesc.MiscFlags			=0;
	texDesc.Usage				=D3D11_USAGE_IMMUTABLE;
	texDesc.Width				=w;
	texDesc.Height				=h;
	texDesc.Format				=DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count	=1;
	texDesc.SampleDesc.Quality	=0;

	//get this into a linear chunk
	uint8_t	*pBytes	=malloc(h * w * 4);

	if(rowPitch > (w * 3))
	{
		//already has alpha?
		for(int y=0;y < h;y++)
		{
			memcpy(pBytes + (y * rowPitch), pRows[y], rowPitch);
		}
	}
	else
	{
		for(int y=0;y < h;y++)
		{
			uint8_t *pRow	=pBytes + (y * (w * 4));

			for(int x=0;x < w;x++)
			{
				int	ofs3	=x * 3;
				int	ofs4	=x * 4;

				pRow[ofs4]		=pRows[y][ofs3];
				pRow[ofs4 + 1]	=pRows[y][ofs3 + 1];
				pRow[ofs4 + 2]	=pRows[y][ofs3 + 2];
				pRow[ofs4 + 3]	=0xFF;
			}
		}
	}

	D3D11_SUBRESOURCE_DATA	sd	={ pBytes, (w * 4), 0 };

	ID3D11Texture2D	*pRet;

	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateTexture2D(pGD->mpDevice1, &texDesc, &sd, &pRet);
	free(pBytes);

	if(hr != S_OK)
	{
		printf("Error creating texture: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11VertexShader	*GD_CreateVertexShader(GraphicsDevice *pGD, const uint8_t *pCodeBytes, int codeLen)
{
	ID3D11VertexShader	*pRet;
	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateVertexShader(pGD->mpDevice1, pCodeBytes, codeLen, NULL, &pRet);
	if(hr != S_OK)
	{
		printf("Error creating vertex shader: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11PixelShader	*GD_CreatePixelShader(GraphicsDevice *pGD, const uint8_t *pCodeBytes, int codeLen)
{
	ID3D11PixelShader	*pRet;
	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreatePixelShader(pGD->mpDevice1, pCodeBytes, codeLen, NULL, &pRet);
	if(hr != S_OK)
	{
		printf("Error creating pixel shader: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11ComputeShader	*GD_CreateComputeShader(GraphicsDevice *pGD, const uint8_t *pCodeBytes, int codeLen)
{
	ID3D11ComputeShader	*pRet;
	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateComputeShader(pGD->mpDevice1, pCodeBytes, codeLen, NULL, &pRet);
	if(hr != S_OK)
	{
		printf("Error creating compute shader: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11ShaderResourceView	*GD_CreateTexSRV(GraphicsDevice *pGD, ID3D11Resource *pRes, DXGI_FORMAT fmt)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC	desc;
	memset(&desc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

	D3D11_TEX2D_SRV		tex	={0, -1};

	desc.Format				=fmt;
	desc.ViewDimension		=D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D			=tex;

	ID3D11ShaderResourceView	*pRet;
	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateShaderResourceView(pGD->mpDevice1,
		pRes, &desc, &pRet);

	if(hr != S_OK)
	{
		printf("Error creating tex SRV: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11ShaderResourceView	*GD_CreateSRV(GraphicsDevice *pGD,
	ID3D11Resource *pRes, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc)
{
	ID3D11ShaderResourceView	*pRet;
	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateShaderResourceView(
		pGD->mpDevice1,
		pRes, pDesc, &pRet);

	if(hr != S_OK)
	{
		printf("Error creating SRV: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11UnorderedAccessView	*GD_CreateUAV(GraphicsDevice *pGD, ID3D11Resource *pRes)
{
	ID3D11UnorderedAccessView	*pRet;
	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateUnorderedAccessView(pGD->mpDevice1,
		pRes, NULL, &pRet);

	if(hr != S_OK)
	{
		printf("Error creating UAV: %X\n", hr);
		return	NULL;
	}
	return	pRet;
}

ID3D11RasterizerState	*GD_CreateRasterizerState(
	GraphicsDevice				*pGD,
	const D3D11_RASTERIZER_DESC	*pDesc)
{
	ID3D11RasterizerState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateRasterizerState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating rast state: %X\n", res);
	return	NULL;
}

ID3D11SamplerState	*GD_CreateSamplerState(
	GraphicsDevice				*pGD,
	const D3D11_SAMPLER_DESC	*pDesc)
{
	ID3D11SamplerState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateSamplerState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating sampler state: %X\n", res);
	return	NULL;
}

ID3D11BlendState	*GD_CreateBlendState(
	GraphicsDevice			*pGD,
	const D3D11_BLEND_DESC	*pDesc)
{
	ID3D11BlendState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateBlendState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating blend state: %X\n", res);
	return	NULL;
}

ID3D11DepthStencilState	*GD_CreateDepthStencilState(
	GraphicsDevice					*pGD,
	const D3D11_DEPTH_STENCIL_DESC	*pDesc)
{
	ID3D11DepthStencilState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateDepthStencilState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating depth/stencil state: %X\n", res);
	return	NULL;
}

ID3D11InputLayout	*GD_CreateInputLayout(
	GraphicsDevice					*pGD,
	const D3D11_INPUT_ELEMENT_DESC	*pIEDs,
	int								numIEDs,
	const void						*byteCode,
	size_t							codeLen)
{
	ID3D11InputLayout	*pRet;

	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateInputLayout(pGD->mpDevice1, pIEDs, numIEDs, byteCode, codeLen, &pRet);
	if(hr == S_OK)
	{
		return	pRet;
	}
	printf("Error creating input layout: %X\n", hr);
	return	NULL;
}

ID3D11Buffer	*GD_CreateBufferWithData(
	GraphicsDevice			*pGD,
	const D3D11_BUFFER_DESC	*pDesc,
	const void				*pData,
	size_t					dataSize)
{
	ID3D11Buffer	*pRet;

	D3D11_SUBRESOURCE_DATA	sd	={ pData, 0, 0 };

	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateBuffer(pGD->mpDevice1, pDesc, &sd, &pRet);
	if(hr == S_OK)
	{
		return	pRet;
	}
	printf("Error creating vertex buffer: %X\n", hr);
	return	NULL;
}

ID3D11Buffer	*GD_CreateBuffer(
	GraphicsDevice			*pGD,
	const D3D11_BUFFER_DESC	*pDesc)
{
	ID3D11Buffer	*pRet;

	HRESULT	hr	=pGD->mpDevice1->lpVtbl->CreateBuffer(pGD->mpDevice1, pDesc, NULL, &pRet);
	if(hr == S_OK)
	{
		return	pRet;
	}
	printf("Error creating buffer: %X\n", hr);
	return	NULL;
}


//set target 0's blend state
void GD_OMSetBlendState(GraphicsDevice *pGD, ID3D11BlendState *pBlend)
{
	pGD->mpContext1->lpVtbl->OMSetBlendState(pGD->mpContext1, pBlend, NULL, 0xFFFFFFFF);
}

void GD_OMSetDepthStencilState(GraphicsDevice *pGD, ID3D11DepthStencilState *pDSS)
{
	pGD->mpContext1->lpVtbl->OMSetDepthStencilState(pGD->mpContext1, pDSS, 0);
}

void GD_ClearDepthStencilView(GraphicsDevice *pGD, ID3D11DepthStencilView *pView)
{
	pGD->mpContext1->lpVtbl->ClearDepthStencilView(pGD->mpContext1,
		pView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void GD_ClearRenderTargetView(GraphicsDevice *pGD,
	ID3D11RenderTargetView *pView, vec4 clearColor)
{
	pGD->mpContext1->lpVtbl->ClearRenderTargetView(pGD->mpContext1,
		pView, clearColor);
}

void GD_OMSetRenderTargets(GraphicsDevice *pGD, ID3D11RenderTargetView *pRTV, ID3D11DepthStencilView *pDSV)
{
	if(pRTV == NULL)
	{
		pGD->mpContext1->lpVtbl->OMSetRenderTargets(
			pGD->mpContext1, 1, NULL, pDSV);
	}
	else
	{
		pGD->mpContext1->lpVtbl->OMSetRenderTargets(
			pGD->mpContext1, 1, &pRTV, pDSV);
	}
}

void GD_IASetInputLayout(GraphicsDevice *pGD, ID3D11InputLayout *pLay)
{
	pGD->mpContext1->lpVtbl->IASetInputLayout(pGD->mpContext1, pLay);
}

void GD_IASetVertexBuffers(GraphicsDevice *pGD,
	ID3D11Buffer *pVB, uint32_t stride, uint32_t offset)
{
	pGD->mpContext1->lpVtbl->IASetVertexBuffers(pGD->mpContext1,
		0, 1, &pVB, &stride, &offset);
}

void GD_IASetIndexBuffers(GraphicsDevice *pGD,
	ID3D11Buffer *pIB, DXGI_FORMAT fmt, uint32_t offset)
{
	pGD->mpContext1->lpVtbl->IASetIndexBuffer(pGD->mpContext1, pIB, fmt, offset);
}

void	GD_UpdateSubResource(GraphicsDevice *pGD,
	ID3D11Resource *pDest, const void *pSrcData)
{
	pGD->mpContext1->lpVtbl->UpdateSubresource(pGD->mpContext1,
		pDest, 0, NULL, pSrcData, 0, 0);

}

void GD_IASetPrimitiveTopology(GraphicsDevice *pGD, D3D11_PRIMITIVE_TOPOLOGY top)
{
	pGD->mpContext1->lpVtbl->IASetPrimitiveTopology(pGD->mpContext1, top);
}

void GD_VSSetShader(GraphicsDevice *pGD, ID3D11VertexShader *pVS)
{
	pGD->mpContext1->lpVtbl->VSSetShader(pGD->mpContext1, pVS, NULL, 0);
}

void GD_VSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf)
{
	pGD->mpContext1->lpVtbl->VSSetConstantBuffers(pGD->mpContext1, slot, 1, &pBuf);
}

void GD_PSSetShader(GraphicsDevice *pGD, ID3D11PixelShader *pPS)
{
	pGD->mpContext1->lpVtbl->PSSetShader(pGD->mpContext1, pPS, NULL, 0);
}

void GD_PSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf)
{
	pGD->mpContext1->lpVtbl->PSSetConstantBuffers(pGD->mpContext1, slot, 1, &pBuf);
}

void GD_CSSetShader(GraphicsDevice *pGD, ID3D11ComputeShader *pCS)
{
	pGD->mpContext1->lpVtbl->CSSetShader(pGD->mpContext1, pCS, NULL, 0);
}

void GD_CSSetConstantBuffer(GraphicsDevice *pGD, int slot, ID3D11Buffer *pBuf)
{
	pGD->mpContext1->lpVtbl->CSSetConstantBuffers(pGD->mpContext1, slot, 1, &pBuf);
}

void GD_CSSetUnorderedAccessViews(GraphicsDevice *pGD,
	UINT startSlot, UINT numUAVs,
	ID3D11UnorderedAccessView * const *ppUAVs,
	const UINT *pUAVInitialCounts)
{	
	pGD->mpContext1->lpVtbl->CSSetUnorderedAccessViews(pGD->mpContext1,
		startSlot, numUAVs, ppUAVs, pUAVInitialCounts);
}

void GD_RSSetState(GraphicsDevice *pGD, ID3D11RasterizerState *pRS)
{
	pGD->mpContext1->lpVtbl->RSSetState(pGD->mpContext1, pRS);
}

void GD_RSSetViewPort(GraphicsDevice *pGD, const D3D11_VIEWPORT *pVP)
{
	pGD->mpContext1->lpVtbl->RSSetViewports(pGD->mpContext1, 1, pVP);
}

void GD_RSSetScissorRects(GraphicsDevice *pGD, UINT numRects, const D3D11_RECT *pRects)
{
	pGD->mpContext1->lpVtbl->RSSetScissorRects(pGD->mpContext1, numRects, pRects);
}

void GD_VSSetSRV(GraphicsDevice *pGD, ID3D11ShaderResourceView *pSRV, int slot)
{
	pGD->mpContext1->lpVtbl->VSSetShaderResources(pGD->mpContext1, slot, 1, &pSRV);
}

void GD_PSSetSRV(GraphicsDevice *pGD, ID3D11ShaderResourceView *pSRV, int slot)
{
	pGD->mpContext1->lpVtbl->PSSetShaderResources(pGD->mpContext1, slot, 1, &pSRV);
}

void GD_CSSetSRV(GraphicsDevice *pGD, ID3D11ShaderResourceView *pSRV, int slot)
{
	pGD->mpContext1->lpVtbl->CSSetShaderResources(pGD->mpContext1, slot, 1, &pSRV);
}

void GD_Dispatch(GraphicsDevice *pGD, UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
{
	pGD->mpContext1->lpVtbl->Dispatch(pGD->mpContext1, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void GD_Draw(GraphicsDevice *pGD, uint32_t vertCount, uint32_t startVert)
{
	pGD->mpContext1->lpVtbl->Draw(pGD->mpContext1, vertCount, startVert);
}

void	GD_PSSetSampler(GraphicsDevice *pGD, ID3D11SamplerState *pSamp, uint32_t slot)
{
	pGD->mpContext1->lpVtbl->PSSetSamplers(pGD->mpContext1, slot, 1, &pSamp);
}

void GD_DrawIndexed(GraphicsDevice *pGD,
	uint32_t indexCount, uint32_t startIndex, uint32_t baseVert)
{
	pGD->mpContext1->lpVtbl->DrawIndexed(pGD->mpContext1, indexCount, startIndex, baseVert);
}

void GD_Present(GraphicsDevice *pGD)
{
	HRESULT	hr	=pGD->mpSwapChain->lpVtbl->Present(pGD->mpSwapChain, 0, DXGI_PRESENT_ALLOW_TEARING);
	if(hr == S_OK)
	{
		return;
	}
	printf("Present error 0x%X\n", hr);
}

void	GD_MapDiscard(GraphicsDevice *pGD, ID3D11Resource *pRS, D3D11_MAPPED_SUBRESOURCE *pMSR)
{
	HRESULT	hr	=pGD->mpContext1->lpVtbl->Map(pGD->mpContext1, pRS, 0, D3D11_MAP_WRITE_DISCARD, 0, pMSR);
	if(hr == S_OK)
	{
		return;
	}
	printf("Map error 0x%X\n", hr);
}

void	GD_UnMap(GraphicsDevice *pGD, ID3D11Resource *pRS)
{
	pGD->mpContext1->lpVtbl->Unmap(pGD->mpContext1, pRS, 0);
}
