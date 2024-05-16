#include	<stdint.h>
#include	<stdio.h>
#include	"../UtilityLib/DictionaryStuff.h"
#include	"../UtilityLib/ListStuff.h"
#include	"Skeleton.h"
#include	"Anim.h"


typedef struct	AnimLib_t
{
	DictSZ	*mpAnims;

	Skeleton	*mpSkeleton;
}	AnimLib;


AnimLib	*AnimLib_Read(const char *fileName)
{
	FILE	*f	=fopen(fileName, "rb");
	if(f == NULL)
	{
		return	NULL;
	}

	uint32_t	magic;
	fread(&magic, sizeof(uint32_t), 1, f);

	if(magic != 0xA91BA7E)
	{
		fclose(f);
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

		UT_string	*szName	=Anim_GetName(pAnim);

		printf("Anim: %s\n", utstring_body(szName));

		DictSZ_Add(&pRet->mpAnims, szName, pAnim);
	}

	fclose(f);

	return	pRet;
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


void	AnimLib_FillBoneArray(const AnimLib *pAL, mat4 *pBones)
{
	Skeleton_FillBoneArray(pAL->mpSkeleton, pBones);
}


const Skeleton	*AnimLib_GetSkeleton(const AnimLib *pAL)
{
	return	pAL->mpSkeleton;
}


int	AnimLib_GetNumAnims(const AnimLib *pAL)
{
	return	DictSZ_Count(pAL->mpAnims);
}


void	AnimNamesCB(const UT_string *pKey, const void *pValue, void *pContext)
{
	StringList	**ppSL	=(StringList **)pContext;

	if(ppSL == NULL)
	{
		return;
	}

	SZList_AddUT(ppSL, pKey);
}

const StringList	*AnimLib_GetAnimList(const AnimLib *pAL)
{
	StringList	*pRet	=SZList_New();

	DictSZ_ForEach(pAL->mpAnims, AnimNamesCB, &pRet);

	return	pRet;
}