#include	<d3d11_1.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdio.h>
#include	<unistd.h>

#define	D3D11_NO_HELPERS
#define	CINTERFACE
#define	COBJMACROS

HWND					sWnd;
ID3D11Device			*spDevice		=NULL;
ID3D11Device1			*spDevice1		=NULL;
ID3D11DeviceContext		*spContext		=NULL;
ID3D11DeviceContext1	*spContext1		=NULL;
IDXGIDevice				*spDXGI			=NULL;
IDXGISwapChain			*spSwapChain	=NULL;
ID3D11RasterizerState	*spRastState	=NULL;


int main(void)
{
	printf("DirectX on looney loonix!\n");

	SDL_Window	*pWnd	=NULL;

	int	res	=SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
	if(res < 0)
	{
		printf("SDL init failed: %s\n", SDL_GetError());
		return	0;
	}

	SDL_Vulkan_LoadLibrary(NULL);

	pWnd	=SDL_CreateWindow("Blortallius!",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 600,
		SDL_WINDOW_VULKAN);
	if(pWnd == NULL)
	{
		printf("Window creation didn't work: %s\n", SDL_GetError());
		SDL_Quit();
		return	0;
	}

	printf("Huzzah!\n");
	
	D3D_FEATURE_LEVEL	retFL, featureLevels[]	={	D3D_FEATURE_LEVEL_11_1	};

	DXGI_SWAP_CHAIN_DESC	scDesc	={};

	scDesc.BufferDesc.Format					=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	scDesc.BufferDesc.RefreshRate.Numerator		=144;
	scDesc.BufferDesc.RefreshRate.Denominator	=1;
	scDesc.SampleDesc.Count						=1;
	scDesc.SampleDesc.Quality					=0;
	scDesc.BufferUsage							=DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow							=pWnd;
	scDesc.Windowed								=TRUE;
	scDesc.BufferCount							=1;
	scDesc.SwapEffect							=DXGI_SWAP_EFFECT_DISCARD;
	scDesc.Flags								=0;

	//device create go!
	HRESULT	hres	=D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		D3D11_CREATE_DEVICE_DEBUG, featureLevels, 1, D3D11_SDK_VERSION,
		&scDesc, &spSwapChain, &spDevice, &retFL, &spContext);
	if(hres != S_OK)
	{
		printf("Error creating device: %dX\n", hres);
		goto	quit;
	}

	//grab a device 1
	hres	=spDevice->lpVtbl->QueryInterface(spDevice, &IID_ID3D11Device1, (void **)&spDevice1);
	if(hres != S_OK)
	{
		printf("Error Querifying Device1: %dX\n", hres);
		goto	quit;
	}

	//grab a DC1
	hres	=spContext->lpVtbl->QueryInterface(spContext,
				&IID_ID3D11DeviceContext1,
				(void **)&spContext1);
	if(hres != S_OK)
	{
		printf("Error Querifying context1: %dX\n", hres);
		goto	quit;
	}

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

	//is this really needed?
	pFact2->lpVtbl->MakeWindowAssociation(pFact2, sWnd,
		DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

	//rasterizer state
	{
		D3D11_RASTERIZER_DESC	desc	={};
		
		desc.FillMode				=D3D11_FILL_SOLID;
		desc.CullMode				=D3D11_CULL_BACK;
		desc.FrontCounterClockwise	=FALSE;
		desc.DepthBias				=0;
		desc.DepthBiasClamp			=0;
		desc.SlopeScaledDepthBias	=0.f;
		desc.DepthClipEnable		=TRUE;
		desc.ScissorEnable			=FALSE;
		desc.MultisampleEnable		=FALSE;
		desc.AntialiasedLineEnable	=FALSE;

		hres	=spDevice->lpVtbl->CreateRasterizerState(spDevice, &desc, &spRastState);
		if(hres != S_OK)
		{
			printf("Error creating rast state: %dX\n", hres);
		}
	}
	sleep(5);

quit:
	SDL_Vulkan_UnloadLibrary();

	SDL_DestroyWindow(pWnd);

	SDL_Quit();

	return	1;
}