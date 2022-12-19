#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
//#include	"AudioLib/Audio.h"	audio stuff not ready yet
#include	"MaterialLib/StuffKeeper.h"
#include	"UtilityLib/GraphicsDevice.h"
#include	"UtilityLib/StringStuff.h"
#include	"UtilityLib/ListStuff.h"
#include	"UtilityLib/DictionaryStuff.h"


//test struct
struct testJunx
{
	int		someVal;
	char	desc[32];
};

//print stuff in DictSZ
void	PrintDictItems(const UT_string *pKey, const void *pValue, void *pContext)
{
	const struct testJunx	*pJunx	=pValue;

	printf("Key: %s, Value: %d, %s\n", utstring_body(pKey), pJunx->someVal, pJunx->desc);
}

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

	//test lists
	StringList	*pList	=SZList_New();

	SZList_Add(&pList, "Splart");
	SZList_Add(&pList, "blort");
	SZList_Add(&pList, "gnarfargeblef");
	SZList_Add(&pList, "braf");
	SZList_Add(&pList, "gnorgleblorgle");
	SZList_Add(&pList, "gack");
	SZList_Add(&pList, "flort");

	printf("List count: %d\n", SZList_Count(pList));
	printf("List contains blort: %d\n", SZList_Contains(pList, "blort"));

	printf("Removing blort...\n");

	SZList_Remove(&pList, "blort");
	printf("List count: %d\n", SZList_Count(pList));
	printf("List contains blort: %d\n", SZList_Contains(pList, "blort"));

	printf("Adding blort again...\n");
	SZList_Add(&pList, "blort");
	printf("List count: %d\n", SZList_Count(pList));
	printf("List contains blort: %d\n", SZList_Contains(pList, "blort"));

	//test removing all, see what happens to the list pointer and such
	SZList_Remove(&pList, "Splart");
	SZList_Remove(&pList, "blort");
	SZList_Remove(&pList, "gnarfargeblef");
	SZList_Remove(&pList, "braf");
	SZList_Remove(&pList, "gnorgleblorgle");
	SZList_Remove(&pList, "gack");
	SZList_Remove(&pList, "flort");

	printf("List count: %d\n", SZList_Count(pList));
	printf("List contains blort: %d\n", SZList_Contains(pList, "blort"));

	//test some dictionary stuff
	DictSZ	*pTestDict;
	DictSZ_New(&pTestDict);

	//some data to test with
	UT_string	*pKey0, *pKey1, *pKey2;

	utstring_new(pKey0);
	utstring_new(pKey1);
	utstring_new(pKey2);

	utstring_printf(pKey0, "%s", "fred");
	utstring_printf(pKey1, "%s", "bob");
	utstring_printf(pKey2, "%s", "jimmy");

	struct testJunx	fredData	={ 69, "slkfdjsdlfjads" };
	struct testJunx	bobData	={ 523, "blarahsdkfjhaskfdh" };
	struct testJunx	jimmyData	={ 78924, "yarrrrrrrrrrrrrrr matey" };

	DictSZ_Add(&pTestDict, pKey0, &fredData);
	DictSZ_Add(&pTestDict, pKey1, &bobData);
	DictSZ_Add(&pTestDict, pKey2, &jimmyData);

	void	*shet	=DictSZ_GetValueccp(pTestDict, "bob");

	DictSZ_ForEach(pTestDict, PrintDictItems, NULL);

	//nuke all
	//test structs are on the stack so no free call
	DictSZ_ClearNoFree(&pTestDict);


	GraphicsDevice	*pGD;

	GraphicsDevice_Init(&pGD, "Blortallius!", 800, 600, D3D_FEATURE_LEVEL_11_0);

	StuffKeeper	*pSK	=StuffKeeper_Create(pGD);

	sleep(5);

	GraphicsDevice_Destroy(&pGD);

	return	EXIT_SUCCESS;
}