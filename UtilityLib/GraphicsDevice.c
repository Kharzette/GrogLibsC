#include	<d3d11.h>
#include	<SDL3/SDL.h>
#include	<SDL3/SDL_vulkan.h>
#include	<stdint.h>
#include	<stdio.h>
#include	<unistd.h>

ID3D11Device		*spDevice	=NULL;
ID3D11DeviceContext	*spContext	=NULL;


int main(void)
{
	printf("DirectX on looney loonix!\n");

	SDL_Window	*pWnd	=NULL;

	int	res	=SDL_Init(SDL_INIT_VIDEO);
	if(res < 0)
	{
		printf("SDL init failed: %s\n", SDL_GetError());
		return	0;
	}

	pWnd	=SDL_CreateWindow("Blortallius!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
	if(pWnd == NULL)
	{
		printf("Window creation didn't work: %s\n", SDL_GetError());
		SDL_Quit();
		return	0;
	}

	SDL_Vulkan_LoadLibrary(NULL);

	printf("Huzzah!\n");
	
	D3D_FEATURE_LEVEL	featureLevels[]	={	D3D_FEATURE_LEVEL_11_0	};

	HRESULT	hres	=D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		D3D11_CREATE_DEVICE_DEBUG, NULL, 1, D3D11_SDK_VERSION,
		&spDevice, NULL, &spContext);

	sleep(5);

	SDL_Vulkan_UnloadLibrary();

	SDL_DestroyWindow(pWnd);

	SDL_Quit();

	return	1;
}

//int	SDL_main(int argc, char *argv[])
//{
//	SDL_Init()
//}