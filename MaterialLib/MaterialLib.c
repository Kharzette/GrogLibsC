#include	<stdint.h>
#include	<stdio.h>
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/StringStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"Material.h"
#include	"StuffKeeper.h"


typedef struct	MaterialLib_t
{
	DictSZ	*mpMats;

	StuffKeeper	*mpSKeeper;

}	MaterialLib;


MaterialLib	*MatLib_Read(const char *pFileName, StuffKeeper *pSK)
{
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

int	MatLib_GetNumMats(const MaterialLib *pML)
{
	return	DictSZ_Count(pML->mpMats);
}


void	MatNamesCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	StringList	**ppSL	=(StringList **)pContext;

	if(ppSL == NULL)
	{
		return;
	}

	SZList_AddUT(ppSL, pKey);
}

const StringList	*MatLib_GetMatList(const MaterialLib *pML)
{
	StringList	*pRet	=SZList_New();

	DictSZ_ForEach(pML->mpMats, MatNamesCB, &pRet);

	return	pRet;
}

Material	*MatLib_GetMaterial(MaterialLib *pML, const char *szMat)
{
	if(!DictSZ_ContainsKeyccp(pML->mpMats, szMat))
	{
		return	NULL;
	}

	return	DictSZ_GetValueccp(pML->mpMats, szMat);
}

const Material	*MatLib_GetConstMaterial(const MaterialLib *pML, const char *szMat)
{
	if(!DictSZ_ContainsKeyccp(pML->mpMats, szMat))
	{
		return	NULL;
	}

	return	DictSZ_GetValueccp(pML->mpMats, szMat);
}