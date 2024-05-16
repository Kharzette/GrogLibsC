#include	<stdint.h>
#include	<stdio.h>
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"Material.h"
#include	"StuffKeeper.h"


typedef struct	MaterialLib_t
{
	DictSZ	*mpMats;

	StuffKeeper	*mpSKeeper;

}	MaterialLib;


MaterialLib	*MatLib_Read(const char *pFileName, const StuffKeeper *pSK)
{
	printf("What the hell is going on\n");
	FILE	*f	=fopen(pFileName, "rb");
	if(f == NULL)
	{
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);

	if(magic != 0xFA77DA77)
	{
		fclose(f);
		return	NULL;
	}

	MaterialLib	*pRet	=malloc(sizeof(MaterialLib));

	pRet->mpSKeeper	=pSK;

	DictSZ_New(&pRet->mpMats);

	int	numMats;
	fread(&numMats, sizeof(int), 1, f);

	for(int i=0;i < numMats;i++)
	{
		UT_string	*szMatName	=SZ_ReadString(f);

		Material	*pM	=MAT_Read(f, pSK);

		DictSZ_Add(&pRet->mpMats, szMatName, pM);
	}

	fclose(f);

	return	pRet;
}