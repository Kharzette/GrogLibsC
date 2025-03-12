#include	<stdint.h>
#include	<stdio.h>
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"Skeleton.h"
#include	"Anim.h"
#include	"AnimLib.h"


//should match CommonFunctions.hlsli
#define	MAX_BONES			55

typedef struct	AnimLib_t
{
	DictSZ	*mpAnims;

	Skeleton	*mpSkeleton;
}	AnimLib;


//static forward decs
static void sMakeMapCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sSaveAnimsCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sSetBoneRefsCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sAnimNamesCB(const UT_string *pKey, const void *pValue, void *pContext);


AnimLib	*AnimLib_Create(Skeleton *pSkel)
{
	AnimLib	*pRet	=malloc(sizeof(AnimLib));

	pRet->mpSkeleton	=pSkel;

	DictSZ_New(&pRet->mpAnims);

	return	pRet;
}

AnimLib	*AnimLib_Read(const char *fileName)
{
	FILE	*f	=fopen(fileName, "rb");
	if(f == NULL)
	{
		printf("Couldn't open file %s\n", fileName);
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);

	if(magic != 0xA91BA7E)
	{
		fclose(f);
		printf("Bad magic for AnimLib_Read() %s\n", fileName);
		return	NULL;
	}

	AnimLib	*pRet	=malloc(sizeof(AnimLib));

	DictSZ_New(&pRet->mpAnims);

	pRet->mpSkeleton	=Skeleton_Read(f);

	int	numAnims;
	fread(&numAnims, sizeof(int), 1, f);

	for(int i=0;i < numAnims;i++)
	{
		Anim	*pAnim	=Anim_Read(f, pRet->mpSkeleton);

		const UT_string	*szName	=Anim_GetName(pAnim);

		printf("Anim: %s\n", utstring_body(szName));

		DictSZ_Add(&pRet->mpAnims, szName, pAnim);
	}

	fclose(f);

	return	pRet;
}

void	AnimLib_Write(const AnimLib *pAL, const char *szFileName)
{
	FILE	*f	=fopen(szFileName, "wb");
	if(f == NULL)
	{
		printf("Couldn't open %s for writing.\n", szFileName);
		return;
	}

	uint32_t	magic	=0xA91BA7E;
	fwrite(&magic, sizeof(uint32_t), 1, f);

	Skeleton_Write(pAL->mpSkeleton, f);

	int	anmCount	=DictSZ_Count(pAL->mpAnims);
	fwrite(&anmCount, sizeof(int), 1, f);

	DictSZ_ForEach(pAL->mpAnims, sSaveAnimsCB, f);

	fclose(f);
}


void	AnimLib_Animate(AnimLib *pAL, const char *szAnimName, float time)
{
	if(!DictSZ_ContainsKeyccp(pAL->mpAnims, szAnimName))
	{
		return;
	}

	Anim	*pAnim	=DictSZ_GetValueccp(pAL->mpAnims, szAnimName);

	Anim_Animate(pAnim, time);
}

void	AnimLib_FillBoneArray(const AnimLib *pAL, mat4 *pBones, int numBones)
{
	Skeleton_FillBoneArray(pAL->mpSkeleton, pBones, numBones);
}


const Skeleton	*AnimLib_GetSkeleton(const AnimLib *pAL)
{
	return	pAL->mpSkeleton;
}

int	AnimLib_GetNumAnims(const AnimLib *pAL)
{
	return	DictSZ_Count(pAL->mpAnims);
}

StringList	*AnimLib_GetAnimList(const AnimLib *pAL)
{
	StringList	*pRet	=SZList_New();

	DictSZ_ForEach(pAL->mpAnims, sAnimNamesCB, &pRet);

	return	pRet;
}


//make sure all anims point to the right bones
void	AnimLib_SetBoneRefs(AnimLib *pAL)
{
	DictSZ_ForEach(pAL->mpAnims, sSetBoneRefsCB, pAL);
}

void	AnimLib_Add(AnimLib *pALib, Anim *pAnim)
{
	DictSZ_Add(&pALib->mpAnims, Anim_GetName(pAnim), pAnim);

	AnimLib_SetBoneRefs(pALib);
}

//get a mapping array that maps bones from foreign to local
const SkellyMap	*AnimLib_GetMapping(const AnimLib *pALib, const Skeleton *pForeignSkel)
{
	SkellyMap	*pSM	=malloc(sizeof(SkellyMap));

	memset(pSM, 0, sizeof(SkellyMap));

	pSM->mpLocalSkel	=pALib->mpSkeleton;
	pSM->mpForeignSkel	=pForeignSkel;

	DictSZ_ForEach(pForeignSkel->mpNameToIndex, sMakeMapCB, pSM);

	return	pSM;
}

//adapt anim from another skeleton
//foreign is non const because we might adopt it
void	AnimLib_AddForeign(AnimLib *pALib, Anim *pAnim, Skeleton *pForeignSkel)
{
	int	locBCnt	=DictSZ_Count(pALib->mpSkeleton->mpNameToIndex);
	int	forBCnt	=DictSZ_Count(pForeignSkel->mpNameToIndex);

	if(locBCnt != forBCnt)
	{
		printf("Warning, skeletons are different!\n");

		if(locBCnt < forBCnt)
		{
			printf("Foreign skeleton is more complex, so using it.\n");

			Skeleton	*pOld	=pALib->mpSkeleton;
			pALib->mpSkeleton	=pForeignSkel;

			//will this actually work!?
			AnimLib_AddForeign(pALib, pAnim, pOld);			
			return;
		}
	}

	SkellyMap	sm;

	sm.mpLocalSkel		=pALib->mpSkeleton;
	sm.mpForeignSkel	=pForeignSkel;

	DictSZ_ForEach(pForeignSkel->mpNameToIndex, sMakeMapCB, &sm);

	Anim_ReMapBoneIndexes(pAnim, sm.mBoneMap);

	DictSZ_Add(&pALib->mpAnims, Anim_GetName(pAnim), pAnim);

	AnimLib_SetBoneRefs(pALib);
}

void	AnimLib_ReName(AnimLib *pAL, const char *szOld, const char *szNew)
{
	if(!DictSZ_ContainsKeyccp(pAL->mpAnims, szOld))
	{
		return;
	}

	Anim	*pAn	=DictSZ_GetValueccp(pAL->mpAnims, szOld);

	DictSZ_Removeccp(&pAL->mpAnims, szOld);
	DictSZ_Addccp(&pAL->mpAnims, szNew, pAn);

	Anim_SetNameccp(pAn, szNew);
}

void	AnimLib_Delete(AnimLib *pAL, const char *szAnim)
{
	if(!DictSZ_ContainsKeyccp(pAL->mpAnims, szAnim))
	{
		return;
	}

	Anim	*pAn	=DictSZ_GetValueccp(pAL->mpAnims, szAnim);

	DictSZ_Removeccp(&pAL->mpAnims, szAnim);

	Anim_Destroy(pAn);
}


//statics
static void	sAnimNamesCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	StringList	**ppSL	=(StringList **)pContext;

	if(ppSL == NULL)
	{
		return;
	}

	SZList_AddUT(ppSL, pKey);
}

static void	sSetBoneRefsCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	AnimLib	*pAL	=(AnimLib *)pContext;
	if(pAL == NULL)
	{
		printf("Null AnimLib in SetBoneRefs!\n");
		return;
	}

	Anim	*pAnim	=(Anim *)pValue;
	if(pAnim == NULL)
	{
		printf("Null Anim in SetBoneRefs!\n");
		return;
	}

	Anim_SetBoneRefs(pAnim, pAL->mpSkeleton);
}

static void	sSaveAnimsCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	Anim	*pAnm	=(Anim *)pValue;
	if(pAnm == NULL)
	{
		return;
	}

	FILE	*f	=(FILE *)pContext;
	if(f == NULL)
	{
		return;
	}

	Anim_Write(pAnm, f);
}

static void sMakeMapCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	SkellyMap	*pSM	=(SkellyMap *)pContext;

	if(pSM == NULL)
	{
		printf("Null SkellyMap in sMakeMapCB!\n");
		return;
	}

	if(!DictSZ_ContainsKey(pSM->mpForeignSkel->mpNameToIndex, pKey))
	{
		//key doesn't exist in skeleton, no big deal, anim won't ref it
	}
	else
	{
		int	locIdx	=(int)pValue;

		int	forIdx	=(int)DictSZ_GetValue(pSM->mpForeignSkel->mpNameToIndex, pKey);

		pSM->mBoneMap[forIdx]	=locIdx;
	}
}