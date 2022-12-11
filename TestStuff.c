#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
//#include	"AudioLib/Audio.h"	audio stuff not ready yet
#include	"MaterialLib/StuffKeeper.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"

int main(void)
{
	printf("DirectX on looney loonix!\n");

	char	szOne[64]	={ "Gob.lin.oid.s!" };
	char	szTwo[64]	={ "gob" };
	char	szThree[64]	={ "Gobl" };
	char	szFour[64]	={ "\tGobl" };
	char	szFive[64]	={ "Gob" };
	char	szSix[64]	={ "\nGobl" };
	char	szSeven[64]	={ "\rGobl" };

	printf("Does %s start with %s? : %d\n", szOne, szTwo, SZ_StartsWith(szOne, szTwo));
	printf("Does %s start with %s? : %d\n", szOne, szThree, SZ_StartsWith(szOne, szThree));
	printf("Does %s start with %s? : %d\n", szOne, szFour, SZ_StartsWith(szOne, szFour));
	printf("Does %s start with %s? : %d\n", szOne, szFive, SZ_StartsWith(szOne, szFive));
	printf("Does %s start with %s? : %d\n", szOne, szSix, SZ_StartsWith(szOne, szSix));
	printf("Does %s start with %s? : %d\n", szOne, szSeven, SZ_StartsWith(szOne, szSeven));

	printf("IndexOf . is %d\n", SZ_IndexOf(szOne, '.'));
	printf("LastIndexOf . is %d\n", SZ_LastIndexOf(szOne, '.'));

	GraphicsDevice	*pGD;

	GraphicsDevice_Init(&pGD, "Blortallius!", 800, 600);

	sleep(5);

	GraphicsDevice_Destroy(&pGD);

	return	EXIT_SUCCESS;
}