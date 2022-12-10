#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
//#include	"AudioLib/Audio.h"	audio stuff not ready yet
#include	"MaterialLib/StuffKeeper.h"
#include	"UtilityLib/GraphicsDevice.h"

int main(void)
{
	printf("DirectX on looney loonix!\n");

	GraphicsDevice	*pGD;

	GraphicsDevice_Init(&pGD, "Blortallius!", 800, 600);

	sleep(5);

	GraphicsDevice_Destroy(&pGD);

	return	EXIT_SUCCESS;
}