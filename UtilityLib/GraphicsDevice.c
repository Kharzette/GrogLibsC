#include	<d3d11.h>
#include	<SDL3/SDL.h>
#include	<stdint.h>
#include	<stdio.h>

ID3D11Device	sDevice;


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

	printf("Huzzah!\n");

	sleep(5);

	SDL_DestroyWindow(pWnd);

	SDL_Quit();

	return	1;
}

//int	SDL_main(int argc, char *argv[])
//{
//	SDL_Init()
//}