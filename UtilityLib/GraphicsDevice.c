#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>

#define	D3D11_NO_HELPERS
#define	CINTERFACE
#define	COBJMACROS


typedef struct GraphicsDevice_t
{
	//D3D interfaces
	HWND					mWnd;
	ID3D11Device			*mpDevice;
	ID3D11Device1			*mpDevice1;
	ID3D11Debug				*mpDebug;
	ID3D11DeviceContext		*mpContext;
	ID3D11DeviceContext1	*mpContext1;
	IDXGISwapChain			*mpSwapChain;

	//featurelevel the device was created with
	D3D_FEATURE_LEVEL	mFeatureLevel;
}	GraphicsDevice;


//try to make a device of the desired feature level
//TODO: get the swap effect stuff right
bool	GraphicsDevice_Init(GraphicsDevice **ppGD, const char *pWindowTitle,
			int w, int h, D3D_FEATURE_LEVEL desiredFeatureLevel)
{
	//alloc
	*ppGD	=malloc(sizeof(GraphicsDevice));
	memset(*ppGD, 0, sizeof(GraphicsDevice));

	GraphicsDevice	*pGD	=*ppGD;

	int	res	=SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
	if(res < 0)
	{
		printf("SDL init failed: %s\n", SDL_GetError());
		return	FALSE;
	}

	SDL_Vulkan_LoadLibrary(NULL);

	pGD->mWnd	=SDL_CreateWindow(pWindowTitle,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		w, h, SDL_WINDOW_VULKAN);
	if(pGD->mWnd == NULL)
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
	scDesc.BufferDesc.RefreshRate.Numerator		=144;
	scDesc.BufferDesc.RefreshRate.Denominator	=1;
	scDesc.SampleDesc.Count						=1;
	scDesc.SampleDesc.Quality					=0;
	scDesc.BufferUsage							=DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow							=pGD->mWnd;	//Use SDL_Window here!
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
		printf("Error creating device & chain: %dX\n", hres);
		return	FALSE;
	}

	//grab a device 1
	hres	=pGD->mpDevice->lpVtbl->QueryInterface(pGD->mpDevice, &IID_ID3D11Device1, (void **)&pGD->mpDevice1);
	if(hres != S_OK)
	{
		printf("Error Querifying Device1: %dX\n", hres);
		goto	quit;
	}

	//grab a DC1
	hres	=pGD->mpContext->lpVtbl->QueryInterface(pGD->mpContext,
				&IID_ID3D11DeviceContext1,
				(void **)&pGD->mpContext1);
	if(hres != S_OK)
	{
		printf("Error Querifying context1: %dX\n", hres);
		goto	quit;
	}
/*
	IDXGIFactory	*pFact;

	hres	=CreateDXGIFactory(&IID_IDXGIFactory, (void **)&pFact);
	if(hres != S_OK)
	{
		printf("Error creating dxgi factory: %dX\n", hres);
		goto	quit;
	}

	IDXGIFactory2	*pFact2;

	hres	=pFact->lpVtbl->QueryInterface(pFact, &IID_IDXGIFactory2, (void **)&pFact2);
	if(hres != S_OK)
	{
		printf("Error creating dxgi factory2: %dX\n", hres);
		goto	quit;
	}
*/
	return	TRUE;

quit:
	SDL_Vulkan_UnloadLibrary();

	SDL_DestroyWindow(pGD->mWnd);

	SDL_Quit();

	return	FALSE;
}

D3D_FEATURE_LEVEL	GraphicsDevice_GetFeatureLevel(GraphicsDevice *pGD)
{
	return	pGD->mFeatureLevel;
}

void	GraphicsDevice_Destroy(GraphicsDevice **ppGD)
{
	GraphicsDevice	*pGD	=*ppGD;

	pGD->mpContext1->lpVtbl->Release(pGD->mpContext1);
	pGD->mpDevice1->lpVtbl->Release(pGD->mpDevice1);
	pGD->mpContext->lpVtbl->Release(pGD->mpContext);
	pGD->mpSwapChain->lpVtbl->Release(pGD->mpSwapChain);
	pGD->mpDevice->lpVtbl->Release(pGD->mpDevice);

	*ppGD	=NULL;

	SDL_Vulkan_UnloadLibrary();

	SDL_DestroyWindow(pGD->mWnd);

	SDL_Quit();
}

ID3D11RasterizerState	*GraphicsDevice_CreateRasterizerState(
	GraphicsDevice			*pGD,
	D3D11_RASTERIZER_DESC	*pDesc)
{
	ID3D11RasterizerState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateRasterizerState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating rast state: %dX\n", res);
	return	NULL;
}

ID3D11BlendState	*GraphicsDevice_CreateBlendState(
	GraphicsDevice		*pGD,
	D3D11_BLEND_DESC	*pDesc)
{
	ID3D11BlendState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateBlendState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating blend state: %dX\n", res);
	return	NULL;
}

ID3D11DepthStencilState	*GraphicsDevice_CreateDepthStencilState(
	GraphicsDevice				*pGD,
	D3D11_DEPTH_STENCIL_DESC	*pDesc)
{
	ID3D11DepthStencilState	*pState;
	HRESULT	res	=pGD->mpDevice1->lpVtbl->CreateDepthStencilState(
		pGD->mpDevice1,	pDesc, &pState);
	
	if(res == S_OK)
	{
		return	pState;
	}
	printf("Error creating depth/stencil state: %dX\n", res);
	return	NULL;
}