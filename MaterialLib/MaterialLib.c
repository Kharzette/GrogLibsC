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


MaterialLib	*MatLib_Create(StuffKeeper *pSK)
{
	MaterialLib	*pRet	=malloc(sizeof(MaterialLib));

	DictSZ_New(&pRet->mpMats);

	pRet->mpSKeeper	=pSK;

	return	pRet;
}

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

typedef struct	MatSaveContext_t
{
	FILE		*f;
	StuffKeeper	*pSK;
}	MatSaveContext;

static void	sWriteMatsCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	MatSaveContext	*pMSC	=(MatSaveContext *)pContext;
	if(pMSC == NULL)
	{
		return;
	}

	Material	*pMat	=(Material *)pValue;
	if(pMat == NULL)
	{
		return;
	}

	SZ_WriteString(pMSC->f, pKey);

	MAT_Write(pMat, pMSC->f, pMSC->pSK);
}

void	MatLib_Write(const MaterialLib *pML, const char *szFileName)
{
	FILE	*f	=fopen(szFileName, "wb");
	if(f == NULL)
	{
		printf("Couldn't open %s for writing.\n", szFileName);
		return;
	}

	uint32_t	magic	=0xFA77DA77;

	fwrite(&magic, sizeof(uint32_t), 1, f);

	int	numMats	=DictSZ_Count(pML->mpMats);
	fwrite(&numMats, sizeof(int), 1, f);

	MatSaveContext	msc;

	msc.f	=f;
	msc.pSK	=pML->mpSKeeper;

	DictSZ_ForEach(pML->mpMats, sWriteMatsCB, &msc);

	fclose(f);
}

void	MatLib_Add(MaterialLib *pML, const char *szName, Material *pMat)
{
	if(DictSZ_ContainsKeyccp(pML->mpMats, szName))
	{
		return;
	}

	DictSZ_Addccp(&pML->mpMats, szName, pMat);
}

void	MatLib_Remove(MaterialLib *pML, const char *szName)
{
	if(!DictSZ_ContainsKeyccp(pML->mpMats, szName))
	{
		return;
	}

	DictSZ_Removeccp(&pML->mpMats, szName);
}

void	MatLib_ReName(MaterialLib *pML, const char *szMatName, const char *szNewName)
{
	if(!DictSZ_ContainsKeyccp(pML->mpMats, szMatName))
	{
		return;
	}

	Material	*pMat	=DictSZ_GetValueccp(pML->mpMats, szMatName);

	DictSZ_Removeccp(&pML->mpMats, szMatName);
	DictSZ_Addccp(&pML->mpMats, szNewName, pMat);
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

StringList	*MatLib_GetMatList(const MaterialLib *pML)
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