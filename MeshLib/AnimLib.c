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

typedef struct	SkeletonChecker_t
{
	Skeleton	*mpForeignSkeleton;

	bool	mbMatch;

	StringList	*mpMissingBones;

}	SkeletonChecker;


//static forward decs
static void sCheckSkelCB(const UT_string *pKey, const void *pValue, void *pContext);
static void sFindMissingBonesCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sSaveAnimsCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sSetBoneRefsCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sAnimNamesCB(const UT_string *pKey, const void *pValue, void *pContext);
static void	sNukeAnimCB(void *pStuff);


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


//see if skeletons match exactly
bool	AnimLib_CheckSkeleton(const AnimLib *pALib, const Skeleton *pForeignSkel)
{
	int	locBCnt	=DictSZ_Count(pALib->mpSkeleton->mpNameToIndex);
	int	forBCnt	=DictSZ_Count(pForeignSkel->mpNameToIndex);

	if(locBCnt != forBCnt)
	{
		return	false;
	}

	SkeletonChecker	sc;

	sc.mbMatch				=true;
	sc.mpForeignSkeleton	=pForeignSkel;
	sc.mpMissingBones		=NULL;	//not used for this check

	DictSZ_ForEach(pALib->mpSkeleton->mpNameToIndex, sCheckSkelCB, &sc);

	return	sc.mbMatch;
}


//adapt anim from another skeleton
void	AnimLib_AddForeign(AnimLib *pALib, Anim *pAnim, const Skeleton *pForeignSkel)
{
	DictSZ	*pForN2I	=pForeignSkel->mpNameToIndex;

	//Probably the best way to do this is to merge the skeletons.
	//If any bones exist in the foreign skeleton that aren't in
	//the animlib's skeleton, merge them in at the next available
	//index.
	SkeletonChecker	sc;

	//for this test, this is backwards
	//the foreign skeleton is iterated and checked against the local
	sc.mpForeignSkeleton	=pALib->mpSkeleton;
	sc.mpMissingBones		=SZList_New();
	sc.mbMatch				=true;

	DictSZ_ForEach(pForN2I, sFindMissingBonesCB, &sc);

	if(sc.mpMissingBones != NULL)
	{
		//found some new bones to add
		//need to figure out where the bone is in the heirarchy
		//then add it to the animlib skeleton in that spot
		//then update the name to index dictionary
		//then remap the bone indexes of the verts referencing the new bone?
		//that's super complicated and annoying, maybe just not allow this?
		//make people fix it in blender?
		for(const StringList *pCur=SZList_Iterate(sc.mpMissingBones);pCur != NULL;pCur=SZList_IteratorNext(pCur))
		{
			const UT_string	*pCurBoneName	=SZList_IteratorValUT(pCur);

			//see what the foreign index is for this bone
			int	forIdx	=(int)DictSZ_GetValue(pForN2I, pCurBoneName);

			printf("AnimLib skeleton is missing bone %s with index %d\n", utstring_body(pCurBoneName), forIdx);	
		}

		printf("Maybe start a new AnimLib with the most complex bone?\n");

		//free
		SZList_Clear(&sc.mpMissingBones);
		return;
	}

	//no new bones, so it should add normally
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

void	AnimLib_Destroy(AnimLib **ppAL)
{
	AnimLib	*pAL	=*ppAL;
	DictSZ_ClearCB(&pAL->mpAnims, sNukeAnimCB);

	Skeleton_Destroy(pAL->mpSkeleton);

	free(pAL);

	*ppAL	=NULL;
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

static void sCheckSkelCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	SkeletonChecker	*pSC	=(SkeletonChecker *)pContext;

	if(pSC == NULL)
	{
		printf("Null Skelcheck in sCheckSkelCB!\n");
		return;
	}

	if(!DictSZ_ContainsKey(pSC->mpForeignSkeleton->mpNameToIndex, pKey))
	{
		printf("Foreign skeleton has no bone %s\n", utstring_body(pKey));
		pSC->mbMatch	=false;
	}
	else
	{
		int	locIdx	=(int)pValue;
		int	forIdx	=(int)DictSZ_GetValue(pSC->mpForeignSkeleton->mpNameToIndex, pKey);

		if(locIdx != forIdx)
		{
			printf("Bone %s has differing index for new animation vs animation lib!\n", utstring_body(pKey));
			pSC->mbMatch	=false;
		}
	}
}

static void sFindMissingBonesCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	SkeletonChecker	*pSC	=(SkeletonChecker *)pContext;

	if(pSC == NULL)
	{
		printf("Null Skelcheck in sFindMissingBonesCB!\n");
		return;
	}

	if(!DictSZ_ContainsKey(pSC->mpForeignSkeleton->mpNameToIndex, pKey))
	{
		printf("Foreign skeleton has no bone %s\n", utstring_body(pKey));
		pSC->mbMatch	=false;

		SZList_AddUT(&pSC->mpMissingBones, pKey);
	}
}

static void	sNukeAnimCB(void *pStuff)
{
	Anim	*pAnim	=(Anim *)pStuff;

	Anim_Destroy(pAnim);
}