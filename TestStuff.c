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
	char	szSix[12]	={ "\n\n\n\n\n\n\nGobl" };
	char	szSeven[64]	={ "\rGobl" };

	printf("Does %s start with %s? : %d\n", szOne, szTwo, SZ_StartsWith(szOne, szTwo));
	printf("Does %s start with %s? : %d\n", szOne, szThree, SZ_StartsWith(szOne, szThree));
	printf("Does %s start with %s? : %d\n", szOne, szFour, SZ_StartsWith(szOne, szFour));
	printf("Does %s start with %s? : %d\n", szOne, szFive, SZ_StartsWith(szOne, szFive));
	printf("Does %s start with %s? : %d\n", szOne, szSix, SZ_StartsWith(szOne, szSix));
	printf("Does %s start with %s? : %d\n", szOne, szSeven, SZ_StartsWith(szOne, szSeven));

	printf("IndexOf . is %d\n", SZ_IndexOf(szOne, '.'));
	printf("LastIndexOf . is %d\n", SZ_LastIndexOf(szOne, '.'));

	UT_string	*pRetOne, *pRetTwo;
	pRetOne	=SZ_GetExtension(szOne);
	pRetTwo	=SZ_GetExtension(szTwo);

	if(pRetOne != NULL)
	{
		printf("Extension of %s is %s\n", szOne, utstring_body(pRetOne));
	}
	else
	{
		printf("Extension of %s is NULL\n", szOne);
	}
	if(pRetTwo != NULL)
	{
		printf("Extension of %s is %s\n", szTwo, utstring_body(pRetTwo));
	}
	else
	{
		printf("Extension of %s is NULL\n", szTwo);
	}

	UT_string	*pTrimTest	=SZ_Trim(szSix);

	printf("Trim Test: %s\n", utstring_body(pTrimTest));

	UT_string	*pPathTest0	=SZ_ConvertPathSlashes("C:\\derp\\blort\\gigaderp/flort.blart");
	UT_string	*pPathTest1	=SZ_ConvertPathSlashes("/derp\\blort\\gigaderp/flort.blart");
	UT_string	*pPathTest2	=SZ_ConvertPathSlashes("derpblortgigaderp/flort.blart");
	UT_string	*pPathTest3	=SZ_ConvertPathSlashes("");

	UT_string	*pPathTestB0	=SZ_ConvertPathBackSlashes("C:/derp/blort/gigaderp\\flort.blart");
	UT_string	*pPathTestB1	=SZ_ConvertPathBackSlashes("\\derp/blort/gigaderp\\flort.blart");
	UT_string	*pPathTestB2	=SZ_ConvertPathBackSlashes("derpblortgigaderp\\flort.blart");
	UT_string	*pPathTestB3	=SZ_ConvertPathBackSlashes("");

	if(pPathTest0 == NULL)
	{
		printf("Test0: NULL\n");
	}
	else
	{
		printf("Test0: %s\n", utstring_body(pPathTest0));
	}

	if(pPathTest1 == NULL)
	{
		printf("Test1: NULL\n");
	}
	else
	{
		printf("Test1: %s\n", utstring_body(pPathTest1));
	}

	if(pPathTest2 == NULL)
	{
		printf("Test2: NULL\n");
	}
	else
	{
		printf("Test2: %s\n", utstring_body(pPathTest2));
	}

	if(pPathTest3 == NULL)
	{
		printf("Test3: NULL\n");
	}
	else
	{
		printf("Test3: %s\n", utstring_body(pPathTest3));
	}

	if(pPathTestB0 == NULL)
	{
		printf("BTest0: NULL\n");
	}
	else
	{
		printf("BTest0: %s\n", utstring_body(pPathTestB0));
	}

	if(pPathTestB1 == NULL)
	{
		printf("BTest1: NULL\n");
	}
	else
	{
		printf("BTest1: %s\n", utstring_body(pPathTestB1));
	}

	if(pPathTestB2 == NULL)
	{
		printf("BTest2: NULL\n");
	}
	else
	{
		printf("BTest2: %s\n", utstring_body(pPathTestB2));
	}

	if(pPathTestB3 == NULL)
	{
		printf("BTest3: NULL\n");
	}
	else
	{
		printf("BTest3: %s\n", utstring_body(pPathTestB3));
	}

	if(pRetOne != NULL)
	{
		utstring_done(pRetOne);
	}
	if(pRetTwo != NULL)
	{
		utstring_done(pRetTwo);
	}
	utstring_done(pTrimTest);

	GraphicsDevice	*pGD;

	GraphicsDevice_Init(&pGD, "Blortallius!", 800, 600);

	sleep(5);

	GraphicsDevice_Destroy(&pGD);

	return	EXIT_SUCCESS;
}